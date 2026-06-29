from Lint.Lint import lint, LintError


@lint("h", "cpp")
def check_no_tabs(filepath, content):
    errors = []
    for lineno, line in enumerate(content.splitlines(), start=1):
        if "\t" in line:
            errors.append(LintError(f"{filepath}:{lineno}: contains tab"))
    if errors:
        raise LintError(errors)