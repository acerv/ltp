# Linux Test Project (core) Framework

# Deprecated libraries

- libipc
- libmsgctl
- libnewipc
- sigwait
- libswap
- vdso_helpers

# Autotools to Meson conversion

When moving to meson build system, a few modifications have been made, some
variables have been dropped and some autotools function have been ignored.
And this is something you should know before compiling (for example) testcases
created with the previous LTP library.

The following paragraph provides this information.

## Unused LTP_CHECK_*

- LTP_CHECK_ACL_SUPPORT: used by tests
- LTP_CHECK_CC_WARN_OLDSTYLE: not needed
- LTP_CHECK_CRYPTO: used by tests
- LTP_CHECK_KERNEL_DEVEL: compile kernel, we don't need it
- LTP_CHECK_LIBMNL: used by tests
- LTP_CHECK_LINUX_PTRACE: only used by ptrace tests
- LTP_CHECK_NOMMU_LINUX: no idea how to use it
- LTP_CHECK_SELINUX: used by tests
- LTP_CHECK_SYSCALL_FCNTL: used by tests
- LTP_CHECK_SYSCALL_NUMA: used by tests
- LTP_CHECK_SYSCALL_SIGNALFD: used by tests
- LTP_DETECT_HOST_CPU: meson has cross-compilation mechanism

## Undefined flags

- HAVE_LIBACL
- HAVE_LIBCRYPTO
- HAVE_LIBMNL

## Flags to merge

- HAVE_LIBCAP / HAVE_SYS_CAPABILITY_H
- HAVE_LIBAIO / HAVE_LIBAIO_H

# TODO

- CI configuration
