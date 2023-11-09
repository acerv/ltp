# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>

subdir('lapi')

compiler = meson.get_compiler('c')
config_data = configuration_data()

###########################
# Declarations
###########################
config_symbols = [
    ['linux/if_link.h', 'IFLA_NET_NS_PID', 'HAVE_DECL_IFLA_NET_NS_PID'],
    ['sys/mman.h',      'MADV_MERGEABLE',  'HAVE_DECL_MADV_MERGEABLE'],
    ['sys/prctl.h',     'PR_CAPBSET_DROP', 'HAVE_DECL_PR_CAPBSET_DROP'],
    ['sys/prctl.h',     'PR_CAPBSET_READ', 'HAVE_DECL_PR_CAPBSET_READ'],
    ['sys/sem.h',       'SEM_STAT_ANY',    'HAVE_DECL_SEM_STAT_ANY'],
]

foreach symbol : config_symbols
    if compiler.has_header_symbol(symbol[0], symbol[1])
        config_data.set(symbol[2], '1')
    endif
endforeach

###########################
# Headers
###########################
config_dataeaders = [
    ['asm/ldt.h',            'HAVE_ASM_LDT_H'],
    ['asm/ptrace.h',         'HAVE_ASM_PTRACE_H'],
    ['cpuid.h',              'HAVE_CPUID_H'],
    ['emmintrin.h',          'HAVE_EMMINTRIN_H'],
    ['fts.h',                'HAVE_FTS_H'],
    ['ifaddrs.h',            'HAVE_IFADDRS_H'],
    ['keyutils.h',           'HAVE_KEYUTILS_H'],
    ['libaio.h',             'HAVE_LIBAIO_H'],
    ['libaio.h',             'HAVE_LIBAIO'],
    ['linux/can.h',          'HAVE_LINUX_CAN_H'],
    ['linux/cgroupstats.h',  'HAVE_LINUX_CGROUPSTATS_H'],
    ['linux/close_range.h',  'HAVE_LINUX_CLOSE_RANGE_H'],
    ['linux/cryptouser.h',   'HAVE_LINUX_CRYPTOUSER_H'],
    ['linux/dccp.h',         'HAVE_LINUX_DCCP_H'],
    ['linux/fsverity.h',     'HAVE_LINUX_FSVERITY_H'],
    ['linux/fs.h',           'HAVE_LINUX_FS_H'],
    ['linux/futex.h',        'HAVE_LINUX_FUTEX_H'],
    ['linux/genetlink.h',    'HAVE_LINUX_GENETLINK_H'],
    ['linux/if_alg.h',       'HAVE_LINUX_IF_ALG_H'],
    ['linux/if_ether.h',     'HAVE_LINUX_IF_ETHER_H'],
    ['linux/if_packet.h',    'HAVE_LINUX_IF_PACKET_H'],
    ['linux/ioprio.h',       'HAVE_LINUX_IOPRIO_H'],
    ['linux/io_uring.h',     'HAVE_LINUX_IO_URING_H'],
    ['linux/keyctl.h',       'HAVE_LINUX_KEYCTL_H'],
    ['linux/mempolicy.h',    'HAVE_LINUX_MEMPOLICY_H'],
    ['linux/module.h',       'HAVE_LINUX_MODULE_H'],
    ['linux/mount.h',        'HAVE_LINUX_MOUNT_H'],
    ['linux/netlink.h',      'HAVE_LINUX_NETLINK_H'],
    ['linux/random.h',       'HAVE_LINUX_RANDOM_H'],
    ['linux/seccomp.h',      'HAVE_LINUX_SECCOMP_H'],
    ['linux/securebits.h',   'HAVE_LINUX_SECUREBITS_H'],
    ['linux/signalfd.h',     'HAVE_LINUX_SIGNALFD_H'],
    ['linux/taskstats.h',    'HAVE_LINUX_TASKSTATS_H'],
    ['linux/tty.h',          'HAVE_LINUX_TTY_H'],
    ['linux/types.h',        'HAVE_LINUX_TYPES_H'],
    ['linux/userfaultfd.h',  'HAVE_LINUX_USERFAULTFD_H'],
    ['linux/vm_sockets.h',   'HAVE_LINUX_VM_SOCKETS_H'],
    ['memory.h',             'HAVE_MEMORY_H'],
    ['netinet/sctp.h',       'HAVE_NETINET_SCTP_H'],
    ['numaif.h',             'HAVE_NUMAIF_H'],
    ['numa.h',               'HAVE_NUMA_H'],
    ['openssl/sha.h',        'HAVE_OPENSSL_SHA_H'],
    ['pthread.h',            'HAVE_PTHREAD_H'],
    ['selinux/selinux.h',    'HAVE_SELINUX_SELINUX_H'],
    ['sys/acl.h',            'HAVE_SYS_ACL_H'],
    ['sys/capability.h',     'HAVE_LIBCAP'],
    ['sys/capability.h',     'HAVE_SYS_CAPABILITY_H'],
    ['sys/epoll.h',          'HAVE_SYS_EPOLL_H'],
    ['sys/fanotify.h',       'HAVE_SYS_FANOTIFY_H'],
    ['sys/inotify.h',        'HAVE_SYS_INOTIFY_H'],
    ['sys/pidfd.h',          'HAVE_SYS_PIDFD_H'],
    ['sys/prctl.h',          'HAVE_SYS_PRCTL_H'],
    ['sys/reg.h',            'HAVE_SYS_REG_H'],
    ['sys/shm.h',            'HAVE_SYS_SHM_H'],
    ['sys/signalfd.h',       'HAVE_SYS_SIGNALFD_H'],
    ['sys/stat.h',           'HAVE_SYS_STAT_H'],
    ['sys/timerfd.h',        'HAVE_SYS_TIMERFD_H'],
    ['sys/types.h',          'HAVE_SYS_TYPES_H'],
    ['sys/ustat.h',          'HAVE_SYS_USTAT_H'],
    ['sys/utsname.h',        'HAVE_SYS_UTSNAME_H'],
    ['sys/xattr.h',          'HAVE_SYS_XATTR_H'],
    ['xfs/xqm.h',            'HAVE_XFS_XQM_H'],
]

foreach header: config_dataeaders
    if compiler.has_header(header[0])
        config_data.set(header[1], '1')
    endif
endforeach

###########################
# Functions
###########################
config_funcs = [
    ['clone3',              'HAVE_CLONE3'],
    ['close_range',         'HAVE_CLOSE_RANGE'],
    ['copy_file_range',     'HAVE_COPY_FILE_RANGE'],
    ['epoll_pwait',         'HAVE_EPOLL_PWAIT'],
    ['epoll_pwait2',        'HAVE_EPOLL_PWAIT2'],
    ['execveat',            'HAVE_EXECVEAT'],
    ['faccessat2',          'HAVE_FACCESSAT2'],
    ['fallocate',           'HAVE_FALLOCATE'],
    ['fsconfig',            'HAVE_FSCONFIG'],
    ['fsopen',              'HAVE_FSOPEN'],
    ['fspick',              'HAVE_FSPICK'],
    ['fstatat',             'HAVE_FSTATAT'],
    ['getauxval',           'HAVE_GETAUXVAL'],
    ['getcontext',          'HAVE_GETCONTEXT'],
    ['getcpu',              'HAVE_GETCPU'],
    ['getdents',            'HAVE_GETDENTS'],
    ['getdents64',          'HAVE_GETDENTS64'],
    ['io_pgetevents',       'HAVE_IO_PGETEVENTS'],
    ['io_uring_enter',      'HAVE_IO_URING_ENTER'],
    ['io_uring_register',   'HAVE_IO_URING_REGISTER'],
    ['io_uring_setup',      'HAVE_IO_URING_SETUP'],
    ['kcmp',                'HAVE_KCMP'],
    ['mkdtemp',             'HAVE_MKDTEMP'],
    ['mallinfo',            'HAVE_MALLINFO'],
    ['mallinfo2',           'HAVE_MALLINFO2'],
    ['mallopt',             'HAVE_MALLOPT'],
    ['modify_ldt',          'HAVE_MODIFY_LDT'],
    ['mount_setattr',       'HAVE_MOUNT_SETATTR'],
    ['move_mount',          'HAVE_MOVE_MOUNT'],
    ['name_to_handle_at',   'HAVE_NAME_TO_HANDLE_AT'],
    ['open_tree',           'HAVE_OPEN_TREE'],
    ['openat',              'HAVE_OPENAT'],
    ['openat2',             'HAVE_OPENAT2'],
    ['pidfd_getfd',         'HAVE_PIDFD_GETFD'],
    ['pidfd_open',          'HAVE_PIDFD_OPEN'],
    ['pidfd_send_signal',   'HAVE_PIDFD_SEND_SIGNAL'],
    ['pkey_mprotect',       'HAVE_PKEY_MPROTECT'],
    ['preadv',              'HAVE_PREADV'],
    ['preadv2',             'HAVE_PREADV2'],
    ['profil',              'HAVE_PROFIL'],
    ['pwritev',             'HAVE_PWRITEV'],
    ['pwritev2',            'HAVE_PWRITEV2'],
    ['quotactl_fd',         'HAVE_QUOTACTL_FD'],
    ['rand_r',              'HAVE_RAND_R'],
    ['recvmmsg',            'HAVE_RECVMMSG'],
    ['renameat2',           'HAVE_RENAMEAT2'],
    ['sched_getcpu',        'HAVE_SCHED_GETCPU'],
    ['sendmmsg',            'HAVE_SENDMMSG'],
    ['sethostid',           'HAVE_SETHOSTID'],
    ['setns',               'HAVE_SETNS'],
    ['sigpending',          'HAVE_SIGPENDING'],
    ['splice',              'HAVE_SPLICE'],
    ['statx',               'HAVE_STATX'],
    ['stime',               'HAVE_STIME'],
    ['sync_file_range',     'HAVE_SYNC_FILE_RANGE'],
    ['syncfs',              'HAVE_SYNCFS'],
    ['tee',                 'HAVE_TEE'],
    ['timerfd_create',      'HAVE_TIMERFD_CREATE'],
    ['timerfd_gettime',     'HAVE_TIMERFD_GETTIME'],
    ['timerfd_settime',     'HAVE_TIMERFD_SETTIME'],
    ['unshare',             'HAVE_UNSHARE'],
    ['ustat',               'HAVE_USTAT'],
    ['utimensat',           'HAVE_UTIMENSAT'],
    ['vmsplice',            'HAVE_VMSPLICE'],
    ['vmsplice',            'HAVE_VMSPLICE'],
]

foreach func : config_funcs
    if compiler.has_function(func[0])
        config_data.set(func[1], '1')
    endif
endforeach

###########################
# Members
###########################
config_members = [
    [
        'struct fanotify_event_info_fid',
        'fsid.__val',
        ['#include <sys/fanotify.h>'],
        'HAVE_STRUCT_FANOTIFY_EVENT_INFO_FID_FSID___VAL'
    ],
    [
        'struct perf_event_mmap_page',
        'aux_head',
        ['#include <linux/perf_event.h>'],
        'HAVE_STRUCT_PERF_EVENT_MMAP_PAGE_AUX_HEAD'
    ],
    [
        'struct sigaction',
        'sa_sigaction',
        ['#include <signal.h>'],
        'HAVE_STRUCT_SIGACTION_SA_SIGACTION'
    ],
    [
        'struct statx',
        'stx_mnt_id',
        [
            '#define _GNU_SOURCE',
            '#include <sys/stat.h>'
        ],
        'HAVE_STRUCT_STATX_STX_MNT_ID'
    ],
    [
        'struct statx',
        'stx_dio_mem_align',
        [
            '#define _GNU_SOURCE',
            '#include <sys/stat.h>'
        ],
        'HAVE_STRUCT_STATX_STX_DIO_MEM_ALIGN'
    ],
    [
        'struct utsname',
        'domainname',
        [
            '#define _GNU_SOURCE',
            '#include <sys/utsname.h>'
        ],
        'HAVE_STRUCT_UTSNAME_DOMAINNAME'
    ],
    [
        'struct taskstats',
        'freepages_count',
        [
            '#include <linux/taskstats.h>',
            '#include <linux/types.h>'
        ],
        'HAVE_STRUCT_TASKSTATS_FREEPAGES_COUNT'
    ],
    [
        'struct taskstats',
        'nvcsw',
        [
            '#include <linux/taskstats.h>',
            '#include <linux/types.h>'
        ],
        'HAVE_STRUCT_TASKSTATS_NVCSW'
    ],
    [
        'struct taskstats',
        'read_bytes',
        [
            '#include <linux/taskstats.h>',
            '#include <linux/types.h>'
        ],
        'HAVE_STRUCT_TASKSTATS_READ_BYTES'
    ],
]

foreach member : config_members
    if compiler.has_member(member[0], member[1], prefix: member[2])
        config_data.set(member[3], '1')
    endif
endforeach

###########################
# Types
###########################
config_types = [
    [
        'enum kcmp_type',
        ['#include <linux/kcmp.h>'],
        'HAVE_ENUM_KCMP_TYPE'
    ],
    [
        'struct acct_v3',
        ['#include <sys/acct.h>'],
        'HAVE_STRUCT_ACCT_V3'
    ],
    [
        'struct af_alg_iv',
        ['#include <linux/if_alg.h>'],
        'HAVE_STRUCT_AF_ALG_IV'
    ],
    [
        'struct sockaddr_alg',
        ['#include <linux/if_alg.h>'],
        'HAVE_STRUCT_SOCKADDR_ALG'
    ],
    [
        'struct fanotify_event_info_fid',
        ['#include <sys/fanotify.h>'],
        'HAVE_STRUCT_FANOTIFY_EVENT_INFO_FID'
    ],
    [
        'struct fanotify_event_info_error',
        ['#include <sys/fanotify.h>'],
        'HAVE_STRUCT_FANOTIFY_EVENT_INFO_ERROR'
    ],
    [
        'struct fanotify_event_info_header',
        ['#include <sys/fanotify.h>'],
        'HAVE_STRUCT_FANOTIFY_EVENT_INFO_HEADER'
    ],
    [
        'struct fanotify_event_info_pidfd',
        ['#include <sys/fanotify.h>'],
        'HAVE_STRUCT_FANOTIFY_EVENT_INFO_PIDFD'
    ],
    [
        'struct file_dedupe_range',
        ['#include <linux/fs.h>'],
        'HAVE_STRUCT_FILE_DEDUPE_RANGE'
    ],
    [
        'struct file_handle',
        [
            '#define _GNU_SOURCE',
            '#include <fcntl.h>'
        ],
        'HAVE_STRUCT_FILE_HANDLE'
    ],
    [
        'struct fs_quota_statv',
        ['#include <xfs/xqm.h>'],
        'HAVE_STRUCT_FS_QUOTA_STATV'
    ],
    [
        'struct if_nextdqblk',
        ['#include <linux/quota.h>'],
        'HAVE_STRUCT_IF_NEXTDQBLK'
    ],
    [
        'struct iovec',
        ['#include <sys/uio.h>'],
        'HAVE_STRUCT_IOVEC'
    ],
    [
        'struct ipc64_perm',
        ['#include <sys/ipcbuf.h>'],
        'HAVE_STRUCT_IPC64_PERM'
    ],
    [
        'struct loop_config',
        ['#include <linux/loop.h>'],
        'HAVE_STRUCT_LOOP_CONFIG'
    ],
    [
        'struct mmsghdr',
        [
            '#define _GNU_SOURCE',
            '#include <sys/types.h>',
            '#include <sys/socket.h>'
        ],
        'HAVE_STRUCT_MMSGHDR'
    ],
    [
        'struct msqid64_ds',
        ['#include <sys/msgbuf.h>'],
        'HAVE_STRUCT_MSQID64_DS'
    ],
    [
        'struct rlimit64',
        [
            '#define _LARGEFILE64_SOURCE',
            '#include <sys/resource.h>'
        ],
        'HAVE_STRUCT_RLIMIT64'
    ],
    [
        'struct semid64_ds',
        ['#include <sys/sem.h>'],
        'HAVE_STRUCT_SEMID64_DS'
    ],
    [
        'struct shmid64_ds',
        ['#include <sys/shmbuf.h>'],
        'HAVE_STRUCT_SHMID64_DS'
    ],
    [
        'struct statx',
        [
            '#define _GNU_SOURCE',
            '#include <sys/stat.h>'
        ],
        'HAVE_STRUCT_STATX'
    ],
    [
        'struct statx_timestamp',
        [
            '#define _GNU_SOURCE',
            '#include <sys/stat.h>'
        ],
        'HAVE_STRUCT_STATX_TIMESTAMP'
    ],
    [
        'struct termio',
        ['#include <sys/ioctl.h>'],
        'HAVE_STRUCT_TERMIO'
    ],
    [
        'struct tpacket_req3',
        ['#include <linux/if_packet.h>'],
        'HAVE_STRUCT_TPACKET_REQ3'
    ],
    [
        'struct user_desc',
        ['#include <asm/ldt.h>'],
        'HAVE_STRUCT_USER_DESC'
    ],
    [
        'struct modify_ldt_ldt_s',
        ['#include <asm/ldt.h>'],
        'HAVE_STRUCT_MODIFY_LDT_LDT_S'
    ],
    [
        'struct xt_entry_match',
        [
            '#include <sys/types.h>',
            '#include <netinet/in.h>',
            '#include <net/if.h>',
            '#include <limits.h>',
            '#include <linux/netfilter_ipv4/ip_tables.h>',
        ],
        'HAVE_STRUCT_XT_ENTRY_MATCH'
    ],
    [
        'struct xt_entry_target',
        [
            '#include <sys/types.h>',
            '#include <netinet/in.h>',
            '#include <net/if.h>',
            '#include <limits.h>',
            '#include <linux/netfilter_ipv4/ip_tables.h>',
        ],
        'HAVE_STRUCT_XT_ENTRY_TARGET'
    ],
    [
        'struct __kernel_old_timeval',
        ['#include <sys/socket.h>'],
        'HAVE_STRUCT___KERNEL_OLD_TIMEVAL'
    ],
    [
        'struct __kernel_old_timespec',
        ['#include <sys/socket.h>'],
        'HAVE_STRUCT___KERNEL_OLD_TIMESPEC'
    ],
    [
        'struct __kernel_timespec',
        ['#include <sys/socket.h>'],
        'HAVE_STRUCT___KERNEL_TIMESPEC'
    ],
    [
        'struct __kernel_old_itimerspec',
        ['#include <sys/socket.h>'],
        'HAVE_STRUCT___KERNEL_OLD_ITIMERSPEC'
    ],
    [
        'struct __kernel_itimerspec',
        ['#include <sys/socket.h>'],
        'HAVE_STRUCT___KERNEL_ITIMERSPEC'
    ],
    [
        'struct futex_waitv',
        ['#include <linux/futex.h>'],
        'HAVE_STRUCT_FUTEX_WAITV'
    ],
    [
        'struct mount_attr',
        [
            config_data.has('HAVE_LINUX_MOUNT_H') ?
                '#include <linux/mount.h>' :
                '#include <sys/mount.h>',
        ],
        'HAVE_STRUCT_MOUNT_ATTR'
    ],
    [
        'struct fsverity_enable_arg',
        ['#include <linux/fsverity.h>'],
        'HAVE_STRUCT_FSVERITY_ENABLE_ARG'
    ],
]

foreach type : config_types
    if compiler.has_type(type[0], prefix: type[1])
        config_data.set(type[2], '1')
    endif
endforeach

###########################
# Compiler built-ins
###########################
code = '''
int main(void) {
	int i = 0, j = 0;
	__atomic_add_fetch(&i, 1, __ATOMIC_ACQ_REL);
	__atomic_load_n(&i, __ATOMIC_SEQ_CST);
	__atomic_store_n(&i, 0, __ATOMIC_RELAXED);
	return i;
}
'''
if compiler.compiles(code, name : '__atomic_*')
    config_data.set('HAVE_ATOMIC_MEMORY_MODEL', 1)
endif

code = '''
int main(void) {
	char arr[16];
	__builtin___clear_cache(arr, arr + sizeof(arr));
    return 0;
}
'''
if compiler.compiles(code, name : '__builtin___clear_cache')
    config_data.set('HAVE_BUILTIN_CLEAR_CACHE', 1)
endif

code = '''
int main(void) {
	int i = 0;
	return __sync_add_and_fetch(&i, 1);
}
'''
if compiler.compiles(code, name : '__sync_add_and_fetch')
    config_data.set('HAVE_SYNC_ADD_AND_FETCH', 1)
endif

###########################
# Supports
###########################
code = '''
#include <sys/capability.h>
#include <linux/types.h>
int main(void) {
	__u16 a;
	__u32 b;
	return 0;
}
'''
if compiler.compiles(code, name : 'newer libcap-2 installed')
    config_data.set('HAVE_NEWER_LIBCAP', 1)
endif

if config_data.has('HAVE_LIBAIO_H')
    code = '''
    #include <libaio.h>
    int main() {
        int result = (int)&io_set_eventfd;
        return result;
    }
    '''
    if compiler.compiles(code, args : '-laio', name : 'io_set_eventfd')
        config_data.set('HAVE_IO_SET_EVENTFD', 1)
    endif
endif

config_h = configure_file(
    output : 'config.h',
    configuration : config_data,
)

ltp_sources += config_h

#############################
# Generate ltp-version.h
#############################
res = run_command(['git', 'describe'], capture : true, check : false)

if res.returncode() == 0
    version = res.stdout()
else
    fs = import('fs')
    version = fs.read(meson.source_root() + '/VERSION')
endif

version = version.strip()

message('Version: ' + version)

ltp_version_h = configure_file(
    input : 'ltp-version.h.in',
    output : 'ltp-version.h',
    configuration : {'version' : version}
)

ltp_sources += ltp_version_h
