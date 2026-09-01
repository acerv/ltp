dnl SPDX-License-Identifier: GPL-2.0-or-later
dnl Copyright (c) 2016 Fujitsu Ltd.
dnl Copyright (c) 2017 Petr Vorel <pvorel@suse.cz>
dnl Author: Xiao Yang <yangx.jy@cn.fujitsu.com>

AC_DEFUN([LTP_CHECK_KEYUTILS_SUPPORT], [
	AC_CHECK_LIB([keyutils], [add_key],
	[AC_DEFINE(HAVE_LIBKEYUTILS, 1, [Define to 1 if you have libkeyutils installed.])
	      AC_SUBST(KEYUTILS_LIBS, "-lkeyutils")])

	LTP_KEYCTL_HEADERS=""

	if test "x$ac_cv_header_keyutils_h" = "xyes" && \
	   test "x$ac_cv_lib_keyutils_add_key" = "xyes"; then
		LTP_KEYCTL_HEADERS="#include <keyutils.h>"
	elif test "x$ac_cv_header_linux_keyctl_h" = "xyes"; then
		LTP_KEYCTL_HEADERS="#include <linux/keyctl.h>"
	fi

	AC_CHECK_TYPES([struct keyctl_dh_params, struct keyctl_kdf_params,
			struct keyctl_pkey_query, struct keyctl_pkey_params],,,
			[$LTP_KEYCTL_HEADERS])
])
