from Lint.Lint import lint, fix_for, LintError


@lint("h", "cpp")
def check_no_tabs(filepath, content):
    errors = []
    for lineno, line in enumerate(content.splitlines(), start=1):
        if "\t" in line:
            errors.append(LintError(f"{filepath}:{lineno}: contains tab"))
    if errors:
        raise LintError(errors)


@fix_for(check_no_tabs)
def fix_no_tabs(content):
    # expandtabs resets its column count at each newline, so tabs are expanded
    # to the next 4-column tab stop the same way an editor would render them.
    return content.expandtabs(4)