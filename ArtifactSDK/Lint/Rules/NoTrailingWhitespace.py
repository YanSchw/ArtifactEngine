from Lint.Lint import lint, LintError


@lint("h", "cpp")
def check_no_trailing_whitespace(filepath, content):
    errors = []
    for lineno, line in enumerate(content.splitlines(), start=1):
        if line != line.rstrip():
            errors.append(LintError(f"{filepath}:{lineno}: trailing whitespace"))
    if errors:
        raise LintError(errors)
