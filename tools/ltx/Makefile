# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2023 SUSE LLC <andrea.cervesato@suse.com>
#
# Install script for Linux Test eXecutor

top_srcdir		?= ../..

include $(top_srcdir)/include/mk/env_pre.mk

BASE_DIR		:= $(abspath $(DESTDIR)/$(prefix))

all:
	$(MAKE) -C ltx

install:
	install -m 00775 $(top_srcdir)/tools/ltx/ltx/ltx $(BASE_DIR)/ltx

include $(top_srcdir)/include/mk/generic_leaf_target.mk
