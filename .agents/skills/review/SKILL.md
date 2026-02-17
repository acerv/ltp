---
name: review
description: LTP Patch Review Skill
---

# LTP Patch Review Protocol

You are an agent that performs a deep regression analysis on patches for the
LTP - Linux Test Project.

## Phase 1: Setup

### Step 1.1: Load Core Files

```
Read: .agents/apply-patch.md
Read: .agents/ground-rules.md
```

### Step 1.2: Fetch and Apply Patch

```bash
# Clean state
git checkout <base-branch>
git branch -D review/<name> 2>/dev/null

# Fetch patch
b4 am <message-id>

# Apply patch
git checkout -b review/<name> <base-branch>
git am <mbox-file>
```

### Step 1.3: Identify Patch Type

Check changed files and load corresponding rules:

- `*.c` or `*.h` → Read `.agents/c-tests.md`
- `*.sh` → Read `.agents/shell-tests.md`
- Mixed → Read both

## Phase 2: Commit Message Checks

For EACH commit, verify in this order:

- **2.1 Signed-off-by**: `git log -1 --format=%b | grep "^Signed-off-by:"` → Line exists
- **2.2 Subject line**: `git log -1 --format=%s` → Clear, <72 chars
- **2.3 Body**: `git log -1 --format=%b` → Explains "why"
- **2.4 Fixes tag**: (if fixing bug) → `Fixes:` present

## Phase 3: Build Checks

Run in this EXACT order. Stop on first failure.

- **3.1 Checkpatch**: `make -C <test-dir> check-<testname>` → No errors for this test
- **3.2 Compile**: `make -C <test-dir> <testname>` → Exit code 0
- **3.3 Each patch compiles**: `git checkout HEAD~N && make -C <dir>` → Exit code 0 for each

## Phase 4: Runtime Checks

Run in this EXACT order. Mark UNKNOWN if requires privileges or times out (>60s).

- **4.1 Run -i 0**: `./<test> -i 0` → No TFAIL/TBROK
- **4.2 Run -i 10**: `./<test> -i 10` → No TFAIL/TBROK
- **4.3 Run -i 100**: `./<test> -i 100` → No TFAIL/TBROK

## Phase 5: Code Review (C Tests)

Check EACH rule. Mark ✅, ❌, or N/A.

### Ground Rules (MANDATORY - any ❌ = reject)

- **G1 No kernel bug workarounds**: Read code for workaround comments
- **G2 No sleep-based sync**: `grep -n "sleep\|nanosleep" <file>`
- **G3 Runtime feature detection**: Check for runtime checks, not just `#ifdef`
- **G4 Root only if needed**: Check `.needs_root` matches actual need
- **G5 Cleanup on all paths**: Check cleanup() handles all resources
- **G6 Portable code**: `make check` passes
- **G7 One change per patch**: Review diff scope
- **G8 Staging for unreleased**: Check if kernel feature is released

### C Test Rules

- **C1 SPDX header**: First line is `// SPDX-License-Identifier: GPL-2.0-or-later`
- **C2 Copyright**: Second block has `Copyright`
- **C3 Doc comment**: `/*\` block exists with description
- **C4 Uses tst_test.h**: `grep "tst_test.h" <file>`
- **C5 No main()**: `grep "^int main\|^void main" <file>` returns empty
- **C6 struct tst_test**: `grep "struct tst_test test" <file>`
- **C7 .gitignore entry**: `grep <testname> <dir>/.gitignore`
- **C8 runtest entry**: `grep <testname> runtest/*`
- **C9 SAFE\_\* macros**: No raw syscalls that should use SAFE\_\*
- **C10 Static vars reset**: Static vars in run() are reset or set in setup()
- **C11 Result reporting**: Uses tst_res()/tst_brk() correctly
- **C12 TCONF for unsupported**: Feature unavailable → TCONF, not TFAIL

## Phase 6: Output

ALWAYS output in this EXACT format:

```
## Review: <patch-subject>

### Commits
- <hash1>: <subject1> - Signed-off-by: ✅/❌
- <hash2>: <subject2> - Signed-off-by: ✅/❌

### Build
- Checkpatch: ✅/❌
- Compiles: ✅/❌
- Each patch compiles alone: ✅/❌

### Runtime
- Tests pass (-i 0): ✅/❌/UNKNOWN
- Tests pass (-i 10): ✅/❌/UNKNOWN
- Tests pass (-i 100): ✅/❌/UNKNOWN

### Code Review
- Ground rules: ✅ all pass / ❌ <list violations>
- C test rules: ✅ all pass / ❌ <list violations>

### Issues Found
1. <issue or "None">

### Summary
- Signed-off-by: ✅/❌
- Commit message: ✅/❌
- Applies cleanly: ✅/❌
- Compiles: ✅/❌
- Tests pass (-i 0): ✅/❌/UNKNOWN
- Tests pass (-i 10): ✅/❌/UNKNOWN
- Tests pass (-i 100): ✅/❌/UNKNOWN

### Verdict

**Approved** ✅ / **Needs revision** ❌ / **Needs discussion** ⚠️

<one-line-reason>
```

## Decision Rules

- ANY ground rule violation → **Needs revision** ❌
- Build failure → **Needs revision** ❌
- Test failure (not UNKNOWN) → **Needs revision** ❌
- Missing Signed-off-by → **Needs revision** ❌
- Missing runtest/gitignore → **Needs revision** ❌
- All checks pass → **Approved** ✅
- Uncertain about rule → **Needs discussion** ⚠️
