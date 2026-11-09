from Lint.Lint import lint, fix_for, LintError

def _normalize_tail(content):
    body = content.rstrip()
    if not body:
        return content
    return body + "\n"


@lint("h", "cpp", "glsl")
def check_single_newline_at_end_of_file(filepath, content):
    if content != _normalize_tail(content):
        if not content.endswith("\n"):
            raise LintError(f"{filepath}:{len(content.splitlines())}: missing newline at end of file")
        lineno = len(content.rstrip().splitlines()) + 1
        raise LintError(f"{filepath}:{lineno}: dangling empty lines at end of file")

@fix_for(check_single_newline_at_end_of_file)
def fix_single_newline_at_end_of_file(content):
    return _normalize_tail(content)
