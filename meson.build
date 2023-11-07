project(
    'ltp-root',
    'c',
    license : 'GPL-2.0+',
    version : '20230929',
    default_options : [
        'c_std=gnu99',
        'warning_level=2',
        'buildtype=debugoptimized',
    ],
)

add_project_arguments('-D_FORTIFY_SOURCE=2', language : 'c')
add_project_arguments('-DLTPLIB', language : 'c')
add_project_arguments('-pipe', language : 'c')
add_project_arguments('-W', language : 'c')
add_project_arguments('-fno-strict-aliasing', language : 'c')
add_project_arguments('-Wold-style-definition', language : 'c')

headers = [
    'include',
    'include/old',
]

sources = [
    'lib/cloner.c',
    'lib/get_path.c',
    'lib/parse_opts.c',
    'lib/random_range.c',
    'lib/safe_file_ops.c',
    'lib/safe_macros.c',
    'lib/safe_net.c',
    'lib/safe_pthread.c',
    'lib/safe_stdio.c',
    'lib/self_exec.c',
    'lib/tlibio.c',
    'lib/tst_af_alg.c',
    'lib/tst_ansi_color.c',
    'lib/tst_arch.c',
    'lib/tst_assert.c',
    'lib/tst_bool_expr.c',
    'lib/tst_buffers.c',
    'lib/tst_capability.c',
    'lib/tst_cgroup.c',
    'lib/tst_checkpoint.c',
    'lib/tst_checksum.c',
    'lib/tst_clocks.c',
    'lib/tst_clone.c',
    'lib/tst_cmd.c',
    'lib/tst_coredump.c',
    'lib/tst_cpu.c',
    'lib/tst_crypto.c',
    'lib/tst_device.c',
    'lib/tst_dir_is_empty.c',
    'lib/tst_epoll.c',
    'lib/tst_fill_file.c',
    'lib/tst_fill_fs.c',
    'lib/tst_fips.c',
    'lib/tst_fs_has_free.c',
    'lib/tst_fs_link_count.c',
    'lib/tst_fs_setup.c',
    'lib/tst_fs_type.c',
    'lib/tst_get_bad_addr.c',
    'lib/tst_hugepage.c',
    'lib/tst_ioctl.c',
    'lib/tst_kconfig.c',
    'lib/tst_kernel.c',
    'lib/tst_kvercmp.c',
    'lib/tst_lockdown.c',
    'lib/tst_memutils.c',
    'lib/tst_mkfs.c',
    'lib/tst_module.c',
    'lib/tst_net.c',
    'lib/tst_netdevice.c',
    'lib/tst_parse_opts.c',
    'lib/tst_path_has_mnt_flags.c',
    'lib/tst_pid.c',
    'lib/tst_process_state.c',
    'lib/tst_rand_data.c',
    'lib/tst_res.c',
    'lib/tst_resource.c',
    'lib/tst_rtctime.c',
    'lib/tst_rtnetlink.c',
    'lib/tst_safe_file_at.c',
    'lib/tst_safe_io_uring.c',
    'lib/tst_safe_macros.c',
    'lib/tst_safe_sysv_ipc.c',
    'lib/tst_safe_timerfd.c',
    'lib/tst_sig.c',
    'lib/tst_sig_proc.c',
    'lib/tst_status.c',
    'lib/tst_supported_fs_types.c',
    'lib/tst_sys_conf.c',
    'lib/tst_taint.c',
    'lib/tst_test.c',
    'lib/tst_thread_state.c',
    'lib/tst_timer.c',
    'lib/tst_timer_test.c',
    'lib/tst_tmpdir.c',
    'lib/tst_uid.c',
    'lib/tst_virt.c',
    'lib/tst_wallclock.c',
]

include_dir = include_directories(headers)
thread_dep = dependency('threads')

lib_ltp = library(
    'ltp',
    sources,
    include_directories : include_dir,
    dependencies: thread_dep,
    link_args : ['-lrt'],
    install: true,
)

subdir('lib/tests')
subdir('lib/newlib_tests')
