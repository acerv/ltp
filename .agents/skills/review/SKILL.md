---
name: review
description: LTP Patch Review Skill
---

# LTP Patch Review Protocol

You are an agent that performs a deep regression analysis on patches for the
LTP - Linux Test Project.

## Supported Review Inputs

- **Git commit**: `abc1234` or `HEAD~1`
- **Git branch**: `feature-branch`
- **Multiple commits**: `abc1234..def5678` or `HEAD~3..HEAD`
- **Patchwork URL**: `https://patchwork.ozlabs.org/patch/...`
- **Lore URL**: `https://lore.kernel.org/ltp/...`
- **Local patch file**: `./0001-fix-something.patch`
- **GitHub PR**: `https://github.com/linux-test-project/ltp/pull/123`

## Step 1: Load Core Files (MANDATORY)

Before starting ANY review, MUST load BOTH files in this order:

1. `.agents/apply-patch.md` — Patch download and application instructions
2. `.agents/ground-rules.md` — Mandatory rules for all LTP code

Do NOT proceed until both files have been read.

## Step 2: Identify Patch Type

Examine the changed files and classify the patch:

- `*.c`, `*.h` files → **C test** → Load `.agents/c-tests.md`
- `*.sh` files → **Shell test** → Load `.agents/shell-tests.md`
- `Makefile*`, `configure.ac`, `*.mk` → **Build system** → No additional file
- `*.rst`, `*.md`, `doc/**` → **Documentation** → No additional file
- `.gitignore`, `.github/**` → **Repository config** → No additional file
- Mixed C + Shell → Load BOTH `.agents/c-tests.md` AND `.agents/shell-tests.md`

## Step 3: Review Commit Messages

For EVERY commit (except those containing `--- b4-submit-tracking ---`):

- MUST have `Signed-off-by:` tag
- MUST be clear and follow kernel commit style
- MUST use `[STAGING]` prefix if targeting unreleased kernel features
- MUST have `Fixes: <hash>` if fixing an LTP commit
- MUST have `Fixes: #N` if fixing GitHub issue N

## Step 4: Run Type-Specific Checks

### For C Tests

MUST verify ALL of the following:

1. Patch applies cleanly to latest master
2. Code compiles without errors: `make -C <test-dir>`
3. Test runs with `-i 0` without failures
4. Test runs with `-i 10` without failures
5. Test runs with `-i 100` without failures

**Timeout rule**: If any step takes longer than 60 seconds, STOP and mark
that step as `UNKNOWN` in the review output.

**On failure**: If compilation or test fails, STOP and report the error.
Do NOT attempt to fix the code unless explicitly asked.

### For Shell Tests

MUST verify ALL of the following:

1. Patch applies cleanly to latest master
2. `make check` emits no warnings for the test
3. All rules from `.agents/shell-tests.md` are satisfied

### For Build System / Documentation / Config

MUST verify:

1. Patch applies cleanly to latest master
2. Changes are consistent with existing patterns
3. No unrelated changes included

## Step 5: Output Review Verdict

End EVERY review with this exact format:

```markdown
### Summary

- Signed-off-by: ✅ / ❌
- Commit message: ✅ / ❌
- Applies cleanly: ✅ / ❌ / N/A
- Compiles: ✅ / ❌ / N/A
- Tests pass (-i 0): ✅ / ❌ / UNKNOWN / N/A
- Tests pass (-i 10): ✅ / ❌ / UNKNOWN / N/A
- Tests pass (-i 100): ✅ / ❌ / UNKNOWN / N/A

### Verdict

**Approved** ✅ / **Needs revision** ❌ / **Needs discussion** ⚠️

<explanation>
```

## Error Handling

- If a file fails to load → STOP and report the error
- If a patch does not apply → Report as "Applies cleanly: ❌" and STOP
- If the build fails → Report as "Compiles: ❌" and include the error message
- If tests fail → Report which iteration failed and include the failure output
- If uncertain about a rule → Flag it as "Needs discussion" in the verdict
