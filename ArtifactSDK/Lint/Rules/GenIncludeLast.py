import re

from Lint.Lint import lint, fix_for, LintError

# The .gen.h files define _GENERATED_BODY_<line> macros keyed only by line
# number, and ARTIFACT_CLASS()/STRUCT/ENUM expand whichever definition the
# preprocessor saw last. An #include placed after the .gen.h include can pull
# in another header's .gen.h and silently replace the body this header's own
# types expand, so the .gen.h include must be the last include of the header.

GEN_INCLUDE = re.compile(r'^\s*#\s*include\s+"[^"]*\.gen\.h"')
ANY_INCLUDE = re.compile(r'^\s*#\s*include\b')
COND_OPEN = re.compile(r'^\s*#\s*if(n?def)?\b')
COND_CLOSE = re.compile(r'^\s*#\s*endif\b')


@lint("h")
def check_gen_include_last(filepath, content):
    gen_lineno = None
    errors = []
    for lineno, line in enumerate(content.splitlines(), start=1):
        if GEN_INCLUDE.match(line):
            gen_lineno = lineno
        elif gen_lineno is not None and ANY_INCLUDE.match(line):
            errors.append(LintError(
                f"{filepath}:{lineno}: #include after the .gen.h include on line {gen_lineno}; "
                f"the .gen.h must be the last include of the header"
            ))
    if errors:
        raise LintError(errors)


@fix_for(check_gen_include_last)
def fix_gen_include_last(content):
    lines = content.splitlines(keepends=True)

    gen_idx = next((i for i, line in enumerate(lines) if GEN_INCLUDE.match(line)), None)
    if gen_idx is None:
        return content
    last_idx = max((i for i, line in enumerate(lines)
                    if ANY_INCLUDE.match(line) and not GEN_INCLUDE.match(line)), default=None)
    if last_idx is None or last_idx < gen_idx:
        return content

    gen_line = lines.pop(gen_idx)
    last_idx -= 1

    # If the last include sits inside an #if block, step past its #endif so the
    # .gen.h include is not moved into the conditional.
    depth = 0
    for line in lines[:last_idx + 1]:
        if COND_OPEN.match(line):
            depth += 1
        elif COND_CLOSE.match(line):
            depth -= 1
    insert_at = last_idx + 1
    while depth > 0 and insert_at < len(lines):
        if COND_OPEN.match(lines[insert_at]):
            depth += 1
        elif COND_CLOSE.match(lines[insert_at]):
            depth -= 1
        insert_at += 1

    lines.insert(insert_at, gen_line)
    return "".join(lines)
