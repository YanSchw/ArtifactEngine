from Lint.Lint import lint, LintError


@lint("h", "cpp")
def check_no_tabs(filepath, content):
    for lineno, line in enumerate(content.splitlines(), start=1):
        if "\t" in line:
            raise LintError(f"{filepath}:{lineno}: contains tab")