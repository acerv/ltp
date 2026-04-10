#!/bin/sh -eux
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
#
# Install python3, pip, pwclient and b4 for patchwork CI integration.

if command -v apk >/dev/null; then
	apk add python3 py3-pip py3-setuptools
elif command -v apt >/dev/null; then
	apt install -y --no-install-recommends python3 python3-pip python3-setuptools
elif command -v dnf5 >/dev/null; then
	dnf5 -y install python3 python3-pip python3-setuptools
elif command -v dnf >/dev/null; then
	dnf -y install python3 python3-pip python3-setuptools
elif command -v yum >/dev/null; then
	yum -y install python3 python3-pip python3-setuptools
elif command -v zypper >/dev/null; then
	zypper --non-interactive install python3 python3-pip python3-setuptools
fi

python3 -m pip install --break-system-packages \
	'pwclient @ git+https://github.com/getpatchwork/pwclient' b4 ||
python3 -m pip install \
	'pwclient @ git+https://github.com/getpatchwork/pwclient' b4
