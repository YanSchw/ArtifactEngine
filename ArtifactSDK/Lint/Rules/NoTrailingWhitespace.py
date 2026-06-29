import re

from Lint.Lint import lint, fix_for, LintError


@lint("h", "cpp")
def check_no_trailing_whitespace(filepath, content):
    errors = []
    for lineno, line in enumerate(content.splitlines(), start=1):
        if line != line.rstrip():
            errors.append(LintError(f"{filepath}:{lineno}: trailing whitespace"))
    if errors:
        raise LintError(errors)


@fix_for(check_no_trailing_whitespace)
def fix_no_trailing_whitespace(content):
    # Strip trailing spaces/tabs before each line ending, and at end-of-file.
    return re.sub(r"[ \t]+(\r?\n|\Z)", r"\1", content)
