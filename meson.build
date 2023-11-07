# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>

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

subdir('lib')
subdir('lib/tests')
subdir('lib/newlib_tests')
