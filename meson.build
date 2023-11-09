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

###########################
# Compiler arguments
###########################
add_project_arguments(
    [
        '-DLTPLIB',
        '-pipe',
        '-W',
        '-fno-strict-aliasing',
        '-Wold-style-definition',
    ],
    language : 'c'
)

compiler = meson.get_compiler('c')

code = '''
#include <stdio.h>
int main(void)
{
#if !defined _FORTIFY_SOURCE && defined __OPTIMIZE__ && __OPTIMIZE__
	return 0;
#else
# error Compiling without optimizations
#endif
}
'''
if compiler.compiles(code, name : 'whether to define _FORTIFY_SOURCE=2')
    add_project_arguments('-D_FORTIFY_SOURCE=2', language : 'c')
endif

###########################
# Build library
###########################
ltp_sources = files()
ltp_dependences = []

subdir('include')
subdir('lib')

ltp_include_dir = include_directories(
    'include',
    'include/old'
)

ltp_library = library(
    'ltp',
    ltp_sources,
    include_directories : ltp_include_dir,
    dependencies: ltp_dependences,
    link_args : ['-lrt'],
    install : true,
    install_dir : 'lib',
)

subdir('lib/tests')
subdir('lib/newlib_tests')
