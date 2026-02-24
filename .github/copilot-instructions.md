# GitHub Copilot Instructions for LTP

When reviewing code, providing suggestions, or assisting with development in this repository, follow the project-specific guidelines documented in:

- **Main agent instructions**: `/AGENTS.md`
- **Ground rules (MANDATORY)**: `/.agents/ground-rules.md`
- **C test guidelines**: `/.agents/c-tests.md`
- **Shell test guidelines**: `/.agents/shell-tests.md`
- **Review protocol**: `/.agents/skills/review/SKILL.md`

## Key Review Points

When reviewing pull requests or commits:

1. **Always check** that code follows the guidelines in the above files
2. **For patches**: Apply the review protocol from `/.agents/skills/review/SKILL.md`
3. **For C code**: Verify all rules in `/.agents/c-tests.md` and `/.agents/ground-rules.md`
4. **For shell code**: Verify all rules in `/.agents/shell-tests.md` and `/.agents/ground-rules.md`

Refer to the full files for complete details and examples.
