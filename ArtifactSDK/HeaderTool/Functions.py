import re
from HeaderTool.Properties import (HeaderToolError, SignatureParameter, blank_comments, extract_comment, generate_signature,
                                   cpp_string, parameter_storage, parse_specifiers, split_top_level)

FUNCTION_PATTERN = re.compile(
    r'FUNCTION\s*\((?P<specifiers>(?:[^()]|\([^()]*\))*)\)\s*'
    r'(?P<prefix>(?:(?:static|virtual|inline)\s+)*)'
    r'(?P<ret>[\w:<>,&* ]+?)\s*\b(?P<name>\w+)\s*'
    r'\((?P<params>(?:[^()]|\([^()]*\))*)\)\s*'
    r'(?P<qualifiers>(?:(?:const|override|final|noexcept)\s*)*)'
    r'(?:=\s*0\s*)?[;{]',
    re.MULTILINE
)

PARAMETER_PATTERN = re.compile(r'^(?P<type>.+?)(?:\s+|(?<=[&*]))(?P<name>\w+)\s*(?:=\s*(?P<default>.+))?$', re.DOTALL)

FUNCTION_SPECIFIERS = ["Pure", "Category"]


def find_functions(body: str, context: str):
    code = blank_comments(body)
    matches = list(FUNCTION_PATTERN.finditer(code))
    starts = {m.start() for m in matches}
    for marker in re.finditer(r'\bFUNCTION\s*\(', code):
        if marker.start() not in starts:
            line = code[:marker.start()].count('\n') + 1
            raise HeaderToolError(f"{context}: could not parse the declaration following FUNCTION(...) on line {line} of its body")
    return matches


def _parse_parameters(text: str, context: str) -> list[SignatureParameter]:
    parameters = []
    for part in split_top_level(text.strip()):
        if part == "void":
            continue
        match = PARAMETER_PATTERN.match(part)
        if not match:
            raise HeaderToolError(f"{context}: every parameter needs a name ('{part}')")
        name = match.group('name')
        if name == "ReturnValue" or any(existing.Name == name for existing in parameters):
            raise HeaderToolError(f"{context}: parameter name '{name}' is reserved or used twice")
        storage, is_pointer, is_output = parameter_storage(match.group('type'), f"{context}({name})")
        default = match.group('default')
        parameters.append(SignatureParameter(name, storage, is_pointer, is_output,
                                             None if is_pointer or not default else default.strip()))
    return parameters


def _generate_function(class_name: str, match, body: str) -> tuple[str, list[str]]:
    name = match.group('name')
    context = f"{class_name}::{name}"
    specifiers = parse_specifiers(match.group('specifiers'), context, FUNCTION_SPECIFIERS, "FUNCTION")
    is_static = 'static' in match.group('prefix').split()
    parameters = _parse_parameters(match.group('params'), context)

    return_type = match.group('ret').strip()
    returns = return_type != "void"
    signature = list(parameters)
    if returns:
        storage, is_pointer, _ = parameter_storage(return_type, f"{context}(ReturnValue)")
        signature.insert(0, SignatureParameter("ReturnValue", storage, is_pointer, True))

    var = f"_Function_{name}"
    struct_name = f"_FunctionParams_{name}"
    lines = generate_signature(struct_name, var, name, class_name, signature, context)

    arguments = ", ".join(parameter.argument() for parameter in parameters)
    target = f"{class_name}::{name}" if is_static else f"static_cast<{class_name}*>(self)->{name}"
    call = f"{'params.ReturnValue = ' if returns else ''}{target}({arguments});"
    self_parameter = "Object*" if is_static else "Object* self"
    lines.append(f"{var}.Invoke = []({self_parameter}, void* data) {{ auto& params = *reinterpret_cast<{struct_name}*>(data); (void)params; {call} }};")
    lines.append(f"{var}.IsStatic = {'true' if is_static else 'false'};")
    lines.append(f"{var}.IsPure = {'true' if 'Pure' in specifiers else 'false'};")
    if specifiers.get("Category"):
        lines.append(f"{var}.Category = {cpp_string(specifiers['Category'].strip().strip(chr(34)))};")
    comment = extract_comment(body, match.start(), match.end())
    if comment:
        lines.append(f"{var}.Comment = {cpp_string(comment)};")
    return var, lines


def generate_functions_registration_code(class_name: str, body: str) -> str:
    matches = find_functions(body, class_name)
    if not matches:
        return ""

    names = [m.group('name') for m in matches]
    for name in names:
        if names.count(name) > 1:
            raise HeaderToolError(f"{class_name}::{name}: FUNCTION names must be unique within a class (no overloads)")

    variables = []
    lines = []
    for match in matches:
        var, function_lines = _generate_function(class_name, match, body)
        variables.append(f"&{var}")
        lines += function_lines

    indented = "\n".join("            " + line for line in "\n".join(lines).splitlines())
    return f"""struct InternalRegisterFunctions {{
        InternalRegisterFunctions() {{
{indented}
            ::Function::RegisterTypeFunctions("{class_name}", {{ {", ".join(variables)} }});
        }}
    }};
    inline static InternalRegisterFunctions _InternalRegisterFunctions;
"""
