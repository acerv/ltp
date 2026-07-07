// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verify that KVM's shadow MMU does not reuse shadow pages with a
 * mismatched role when ``kvm_mmu_get_child_sp()`` looks up a child
 * page during nested page fault handling.
 *
 * A use-after-free in the shadow MMU allowed a racing nested page
 * directory entry -- toggled between 2 MiB huge-page and 4 KiB
 * page-table forms -- to trick the host into reusing a shadow page
 * whose ``role.word`` no longer matched, corrupting
 * ``pte_list_remove()`` and crashing the host.
 *
 * The test loads a kernel module that creates nested VMs using raw
 * VMX (Intel) or SVM (AMD) instructions, then races the mapping
 * change against L3 VM page faults. If the host kernel survives
 * without a BUG or taint, the fix is effective.
 *
 * This test is designed to run inside a KVM guest with nested
 * virtualisation enabled. ``kvm_intel`` / ``kvm_amd`` must not be
 * loaded in the guest.
 */

#include "tst_test.h"
#include "tst_module.h"

#define MODULE_NAME	"januscape_mod"
#define MODULE_NAME_KO	MODULE_NAME ".ko"

static int module_loaded;

static void run(void)
{
	tst_requires_module_signature_disabled();

	tst_res(TINFO,
		"Loading module to race shadow MMU page reuse");

	tst_module_load(MODULE_NAME_KO, NULL);
	module_loaded = 1;

	if (tst_taint_check()) {
		tst_res(TFAIL,
			"Kernel tainted: shadow MMU corrupted");
		return;
	}

	tst_res(TPASS, "Shadow MMU race completed without corruption");

	tst_module_unload(MODULE_NAME_KO);
	module_loaded = 0;
}

static void cleanup(void)
{
	if (module_loaded)
		tst_module_unload(MODULE_NAME_KO);
}

static struct tst_test test = {
	.test_all = run,
	.cleanup = cleanup,
	.needs_root = 1,
	.skip_in_lockdown = 1,
	.skip_in_secureboot = 1,
	.taint_check = TST_TAINT_W | TST_TAINT_D,
	.supported_archs = (const char *const []) {
		"x86_64",
		NULL
	},
	.needs_kconfigs = (const char *[]) {
		"CONFIG_KVM",
		"CONFIG_KVM_INTEL | CONFIG_KVM_AMD",
		NULL
	},
	.tags = (const struct tst_tag[]) {
		{"linux-git", "81ccda30b4e8"},
		{"CVE", "2026-53359"},
		{}
	},
};
