# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>

prog_python = find_program('python3')

syscalls_h = custom_target(
    'syscalls.h',
    input : 'syscalls/gen-syscalls.py',
    output : 'syscalls.h',
    command : [prog_python, '@INPUT@', '@OUTPUT@'],
    install : true,
    install_dir : 'include/lapi',
)

ltp_sources += syscalls_h
