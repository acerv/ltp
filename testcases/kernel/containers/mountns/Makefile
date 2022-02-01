# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2014 Red Hat, Inc.
# Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>

top_srcdir		?= ../../../..

LTPLIBS = ltpclone

include $(top_srcdir)/include/mk/testcases.mk

LDLIBS  := -lltpclone $(LDLIBS)

include $(top_srcdir)/include/mk/generic_leaf_target.mk
