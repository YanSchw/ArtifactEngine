from Lint.Lint import lint, fix_for, LintError


def _normalize_tail(content):
    body = content.rstrip()
    tail = content[len(body):]
    if not body or "\n" not in tail:
        return content
    eol = "\r\n" if "\r\n" in tail else "\n"
    return body + eol


@lint("h", "cpp")
def check_no_trailing_empty_lines(filepath, content):
    if content != _normalize_tail(content):
        lineno = len(content.rstrip().splitlines()) + 1
        raise LintError(f"{filepath}:{lineno}: dangling empty lines at end of file")


@fix_for(check_no_trailing_empty_lines)
def fix_no_trailing_empty_lines(content):
    return _normalize_tail(content)