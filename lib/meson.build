# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>

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

config_h = configure_file(
    input : 'ltp-version.h.in',
    output : 'ltp-version.h',
    configuration : {'version' : version}
)

#############################
# Build library
#############################
ltp_headers = [
    '../include',
    '../include/old',
]

ltp_sources = [
    'cloner.c',
    'get_path.c',
    'parse_opts.c',
    'random_range.c',
    'safe_file_ops.c',
    'safe_macros.c',
    'safe_net.c',
    'safe_pthread.c',
    'safe_stdio.c',
    'self_exec.c',
    'tlibio.c',
    'tst_af_alg.c',
    'tst_ansi_color.c',
    'tst_arch.c',
    'tst_assert.c',
    'tst_bool_expr.c',
    'tst_buffers.c',
    'tst_capability.c',
    'tst_cgroup.c',
    'tst_checkpoint.c',
    'tst_checksum.c',
    'tst_clocks.c',
    'tst_clone.c',
    'tst_cmd.c',
    'tst_coredump.c',
    'tst_cpu.c',
    'tst_crypto.c',
    'tst_device.c',
    'tst_dir_is_empty.c',
    'tst_epoll.c',
    'tst_fill_file.c',
    'tst_fill_fs.c',
    'tst_fips.c',
    'tst_fs_has_free.c',
    'tst_fs_link_count.c',
    'tst_fs_setup.c',
    'tst_fs_type.c',
    'tst_get_bad_addr.c',
    'tst_hugepage.c',
    'tst_ioctl.c',
    'tst_kconfig.c',
    'tst_kernel.c',
    'tst_kvercmp.c',
    'tst_lockdown.c',
    'tst_memutils.c',
    'tst_mkfs.c',
    'tst_module.c',
    'tst_net.c',
    'tst_netdevice.c',
    'tst_parse_opts.c',
    'tst_path_has_mnt_flags.c',
    'tst_pid.c',
    'tst_process_state.c',
    'tst_rand_data.c',
    'tst_res.c',
    'tst_resource.c',
    'tst_rtctime.c',
    'tst_rtnetlink.c',
    'tst_safe_file_at.c',
    'tst_safe_io_uring.c',
    'tst_safe_macros.c',
    'tst_safe_sysv_ipc.c',
    'tst_safe_timerfd.c',
    'tst_sig.c',
    'tst_sig_proc.c',
    'tst_status.c',
    'tst_supported_fs_types.c',
    'tst_sys_conf.c',
    'tst_taint.c',
    'tst_test.c',
    'tst_thread_state.c',
    'tst_timer.c',
    'tst_timer_test.c',
    'tst_tmpdir.c',
    'tst_uid.c',
    'tst_virt.c',
    'tst_wallclock.c',
]

include_dir = include_directories(ltp_headers)
thread_dep = dependency('threads')
keyutils_dep = dependency('keyutils', required: false)

lib_ltp = library(
    'ltp',
    ltp_sources + config_h,
    include_directories : include_dir,
    dependencies: [thread_dep, keyutils_dep],
    link_args : ['-lrt'],
    install: true,
)
