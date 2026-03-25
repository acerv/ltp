#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
#
# Deterministic pre-review checks for LTP patch series.
# Runs in CI (GitHub Actions) and reports results to Patchwork.
#
# Checks performed:
#   1. Commit messages  (Signed-off-by, subject length, body)
#   2. make check       (checkpatch for C/H, checkbashisms for shell)
#
# Prerequisites:
#   The tree must be configured (autotools + configure) before running,
#   since `make check-<name>` depends on config.mk.
#
# Usage:
#   patch-precheck.sh -s SERIES_ID -u TARGET_URL [-b BASE]
#
# Environment:
#   PATCHWORK_TOKEN   Required for reporting results to Patchwork

set -e

BASE="master"
SERIES_ID=""
TARGET_URL=""
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOP_SRCDIR="${TOP_SRCDIR:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
PATCHWORK_SH="$SCRIPT_DIR/patchwork.sh"

usage() {
	echo "Usage: $0 -s SERIES_ID -u TARGET_URL [-b BASE]"
	echo ""
	echo "  -s ID    Patchwork series ID"
	echo "  -u URL   CI run URL (for Patchwork target_url)"
	echo "  -b BASE  Base branch (default: master)"
	echo "  -h       Show this help"
	exit 0
}

while getopts "s:u:b:h" opt; do
	case "$opt" in
	s) SERIES_ID="$OPTARG" ;;
	u) TARGET_URL="$OPTARG" ;;
	b) BASE="$OPTARG" ;;
	h) usage ;;
	*) usage ;;
	esac
done

[ -z "$SERIES_ID" ] && {
	echo "ERROR: -s SERIES_ID is required" >&2
	exit 1
}
[ -z "$TARGET_URL" ] && {
	echo "ERROR: -u TARGET_URL is required" >&2
	exit 1
}

# ── helpers ───────────────────────────────────────────────────────────────────

report() {
	local context="$1"
	local state="$2"
	local description="$3"

	echo "[$context] $state: $description"

	if [ -n "$PATCHWORK_TOKEN" ]; then
		"$PATCHWORK_SH" check \
			"$SERIES_ID" "$TARGET_URL" "$context" "$state" "$description"
	fi
}

commits() {
	git rev-list --reverse "${BASE}..HEAD"
}

commit_count() {
	git rev-list --count "${BASE}..HEAD"
}

changed_files() {
	git diff --name-only "${BASE}..HEAD"
}

# ── Check 1: Commit messages ─────────────────────────────────────────────────

check_commit_messages() {
	local overall="success"
	local failures=""

	for sha in $(commits); do
		local subject=$(git log -1 --format=%s "$sha")
		local body=$(git log -1 --format=%b "$sha")
		local short=$(git rev-parse --short "$sha")

		# Signed-off-by
		if ! printf '%s' "$body" | grep -q "^Signed-off-by:"; then
			failures="${failures}${short}: missing Signed-off-by; "
			overall="fail"
		fi

		# Subject length
		if [ ${#subject} -gt 72 ]; then
			failures="${failures}${short}: subject too long (${#subject} chars); "
			overall="fail"
		fi

		# Body present (non-empty after stripping tags)
		local body_text=$(printf '%s' "$body" |
			grep -v "^Signed-off-by:\|^Reviewed-by:\|^Acked-by:\|^Fixes:\|^Cc:\|^Link:" |
			sed '/^$/d')
		if [ -z "$body_text" ]; then
			failures="${failures}${short}: empty commit body; "
			overall="warning"
		fi
	done

	if [ "$overall" = "success" ]; then
		report "precheck_commit-msg" "success" \
			"All $(commit_count) commit(s) have valid messages"
	else
		report "precheck_commit-msg" "$overall" \
			"${failures% ; }"
	fi

	[ "$overall" != "fail" ]
}

# ── Check 2: make check (checkpatch + checkbashisms) ─────────────────────────

check_make_check() {
	local overall="success"
	local failures=""
	local checked=0

	for f in $(changed_files); do
		local name=""
		local target=""

		case "$f" in
		testcases/open_posix_testsuite/*) continue ;;
		*.c)
			name=$(basename "$f" .c)
			target="check-${name}"
			;;
		*.h)
			name=$(basename "$f")
			target="check-${name}"
			;;
		*.sh)
			name=$(basename "$f" .sh)
			target="check-${name}.sh"
			;;
		*) continue ;;
		esac

		local dir=$(dirname "$f")

		[ -f "$TOP_SRCDIR/$dir/Makefile" ] || continue
		[ -f "$TOP_SRCDIR/$f" ] || continue

		checked=$((checked + 1))

		if ! make -C "$TOP_SRCDIR/$dir" "$target" >/dev/null 2>&1; then
			failures="${failures}${name}; "
			overall="fail"
		fi
	done

	if [ "$checked" -eq 0 ]; then
		report "precheck_make-check" "success" "No files to check"
	elif [ "$overall" = "success" ]; then
		report "precheck_make-check" "success" \
			"make check passed for $checked file(s)"
	else
		report "precheck_make-check" "fail" \
			"make check failed: ${failures% ; }"
	fi

	[ "$overall" != "fail" ]
}

# ── Main ──────────────────────────────────────────────────────────────────────

command -v git >/dev/null 2>&1 || {
	echo "ERROR: git not found" >&2
	exit 1
}
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || {
	echo "ERROR: not inside a git repo" >&2
	exit 1
}

N=$(commit_count)
[ "$N" -gt 0 ] || {
	echo "ERROR: no commits between $BASE and HEAD" >&2
	exit 1
}

echo "=== LTP patch precheck: $N commit(s) on $(git rev-parse --abbrev-ref HEAD) ==="

rc=0

check_commit_messages || rc=1
check_make_check || rc=1

if [ "$rc" -eq 0 ]; then
	echo "=== All prechecks passed ==="
else
	echo "=== Some prechecks failed ==="
fi

exit $rc
