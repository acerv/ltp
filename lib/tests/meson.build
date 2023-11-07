# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>

old_tests = [
    'trerrno.c',
    'tst_checkpoint.c',
    'tst_checkpoint_wait_timeout.c',
    'tst_checkpoint_wake_timeout.c',
    'tst_cleanup_once.c',
    'tst_dataroot01.c',
    'tst_dataroot02.c',
    'tst_dataroot03.c',
    'tst_device.c',
    'tst_fs_fill_hardlinks.c',
    'tst_fs_fill_subdirs.c',
    'tst_process_state.c',
    'tst_record_childstatus.c',
    'tst_safe_macros.c',
    'tst_strerrno.c',
    'tst_strsig.c',
    'tst_tmpdir_test.c',
]

foreach src : old_tests
    bin = src.split('.')[0]
    exe = executable(
        bin,
        src,
        include_directories : include_dir,
        dependencies: thread_dep,
        link_args : ['-lrt'],
        link_with : lib_ltp,
    )
    test(bin, exe)
endforeach
