import math
import re

PROPERTY_PATTERN = re.compile(
        r'PROPERTY\s*\((?P<specifiers>(?:[^()]|\([^()]*\))*)\)\s*'
        r'(?P<type>[\w:<>,&* ]+?)'
        r'\s+'
        r'(?P<name>\w+)'
        r'(?:\s*=\s*(?P<default>[^;]+))?'
        r'\s*;',
        re.MULTILINE
    )

KNOWN_SPECIFIERS = ["Range", "Params"]

SIMPLE_PROPERTY = """static {PROPERTY_TYPE} {VAR} = {PROPERTY_TYPE}("{PROPERTY_NAME}", {OFFSET}{INITIALIZER});"""

ARRAY_LAMBDAS = """
/* GetSize */
[](void* ptr) -> size_t {{
    return reinterpret_cast<Array<{PROPERTY_TYPE}>*>(ptr)->Size();
}},
/* GetElementPtr */
[](void* ptr, size_t i)->void* {{
    return &(*reinterpret_cast<Array<{PROPERTY_TYPE}>*>(ptr))[i];
}},
/* AddDefault */
[](void* ptr) {{
    reinterpret_cast<Array<{PROPERTY_TYPE}>*>(ptr)->Emplace();
}},
/* Clear */
[](void* ptr) {{
    reinterpret_cast<Array<{PROPERTY_TYPE}>*>(ptr)->Clear();
}},
/* RemoveAt */
[](void* ptr, size_t i) {{
    reinterpret_cast<Array<{PROPERTY_TYPE}>*>(ptr)->RemoveAt((int32_t)i);
}},
/* Swap */
[](void* ptr, size_t a, size_t b) {{
    reinterpret_cast<Array<{PROPERTY_TYPE}>*>(ptr)->SwapElements((int32_t)a, (int32_t)b);
}}
"""

MAP_LAMBDAS = """
/* GetSize */
[](void* ptr) -> size_t {{
    return reinterpret_cast<{MAP_TYPE}*>(ptr)->Size();
}},
/* GetEntries */
[](void* ptr, Array<MapProperty::Entry>& out) {{
    for (auto& entry : *reinterpret_cast<{MAP_TYPE}*>(ptr)) {{
        out.Add(MapProperty::Entry{{ &entry.first, &entry.second }});
    }}
}},
/* Find */
[](void* ptr, const void* key) -> void* {{
    return reinterpret_cast<{MAP_TYPE}*>(ptr)->Find(*reinterpret_cast<const {KEY_TYPE}*>(key));
}},
/* FindOrAdd */
[](void* ptr, const void* key) -> void* {{
    return &(*reinterpret_cast<{MAP_TYPE}*>(ptr))[*reinterpret_cast<const {KEY_TYPE}*>(key)];
}},
/* Remove */
[](void* ptr, const void* key) {{
    reinterpret_cast<{MAP_TYPE}*>(ptr)->Remove(*reinterpret_cast<const {KEY_TYPE}*>(key));
}},
/* RenameKey */
[](void* ptr, const void* from, const void* to) -> bool {{
    auto& map = *reinterpret_cast<{MAP_TYPE}*>(ptr);
    const {KEY_TYPE}& source = *reinterpret_cast<const {KEY_TYPE}*>(from);
    const {KEY_TYPE}& target = *reinterpret_cast<const {KEY_TYPE}*>(to);
    if (!map.ContainsKey(source) || map.ContainsKey(target)) {{
        return false;
    }}
    {VALUE_TYPE} value = std::move(map[source]);
    map.Remove(source);
    map[target] = std::move(value);
    return true;
}},
/* Clear */
[](void* ptr) {{
    reinterpret_cast<{MAP_TYPE}*>(ptr)->Clear();
}},
/* NewKey */
[]() -> void* {{
    return new {KEY_TYPE}();
}},
/* DeleteKey */
[](void* key) {{
    delete reinterpret_cast<{KEY_TYPE}*>(key);
}}
"""

DELEGATE_LAMBDAS = """
/* GetBindingCount */
[](const void* ptr) -> int32_t {{
    return reinterpret_cast<const {DELEGATE_TYPE}*>(ptr)->GetBindingCount();
}},
/* Bind */
[](void* ptr, Object* owner, std::function<void(void*)> handler) -> DelegateHandle {{
    return reinterpret_cast<{DELEGATE_TYPE}*>(ptr)->Add(owner, [handler]({ARGUMENTS}) {{
        {PARAMS_STRUCT} params;
{ASSIGNMENTS}
        handler(&params);
    }});
}},
/* Unbind */
[](void* ptr, DelegateHandle handle) {{
    reinterpret_cast<{DELEGATE_TYPE}*>(ptr)->Remove(handle);
}}
"""

BUILTIN_STRUCTS = ["Vec2", "Vec3", "Vec4", "Color", "Quat", "UIValue", "UIVec2", "UIPadding"]

INT_TYPES = {
    "uint8_t": (True, 8), "uint8": (True, 8),
    "uint16_t": (True, 16), "uint16": (True, 16),
    "uint32_t": (True, 32), "uint32": (True, 32),
    "uint64_t": (True, 64), "uint64": (True, 64),
    "int8_t": (False, 8), "int8": (False, 8),
    "int16_t": (False, 16), "int16": (False, 16),
    "int32_t": (False, 32), "int32": (False, 32),
    "int64_t": (False, 64), "int64": (False, 64),
}

MAP_KEY_KINDS = {"Int", "Bool", "String", "Enum", "UUID", "SubclassOf"}


class HeaderToolError(Exception):
    pass


class GeneratedProperty:
    def __init__(self, kind: str, var: str, code: str, element: "GeneratedProperty" = None, is_unsigned: bool = False):
        self.Kind = kind
        self.Var = var
        self.Code = code
        self.Element = element
        self.IsUnsigned = is_unsigned

    def numeric_target(self) -> "GeneratedProperty":
        if self.Kind in ("Int", "Float"):
            return self
        return self.Element.numeric_target() if self.Element else None


def _enum_underlying_byte_size(underlying: str) -> int:
    sizes = {
        "uint8_t": 1, "int8_t": 1, "uint8": 1, "int8": 1,
        "uint16_t": 2, "int16_t": 2, "uint16": 2, "int16": 2,
        "uint32_t": 4, "int32_t": 4, "uint32": 4, "int32": 4,
        "uint64_t": 8, "int64_t": 8, "uint64": 8, "int64": 8,
    }
    return sizes.get(underlying, 4)  # default 'int' is 4 bytes


def normalize_type(typename: str) -> str:
    typename = re.sub(r'\s+', ' ', typename.strip())
    return re.sub(r'\s*([<>,&*])\s*', r'\1', typename)


def split_top_level(text: str, separator: str = ',') -> list[str]:
    parts = []
    depth = 0
    current = ''
    for c in text:
        if c in '<([':
            depth += 1
        elif c in '>)]':
            depth -= 1
        if c == separator and depth == 0:
            parts.append(current.strip())
            current = ''
        else:
            current += c
    if current.strip():
        parts.append(current.strip())
    return parts


def _template_argument(typename: str, template: str) -> str:
    return typename[len(template) + 1:-1].strip()


def _is_template(typename: str, template: str) -> bool:
    return typename.startswith(template + "<") and typename.endswith(">")


def cpp_string(text: str) -> str:
    escaped = (text.replace('\\', '\\\\').replace('"', '\\"')
               .replace('\r', '').replace('\n', '\\n').replace('\t', '\\t'))
    return f'"{escaped}"'


def _comment_text(lines: list[str]) -> str:
    paragraphs = []
    current = []
    for line in lines:
        line = line.strip()
        if not line or re.fullmatch(r'[-=*/#~_]+', line):
            if current:
                paragraphs.append(' '.join(current))
                current = []
            continue
        current.append(line)
    if current:
        paragraphs.append(' '.join(current))
    return '\n'.join(paragraphs)


def extract_comment(body: str, start: int, end: int) -> str:
    before = body[:start].split('\n')
    lines = []
    if before[-1].strip() == '':
        index = len(before) - 2
        if index >= 0 and before[index].rstrip().endswith('*/'):
            block = []
            while index >= 0:
                block.insert(0, before[index])
                if '/*' in before[index]:
                    break
                index -= 1
            text = '\n'.join(block)
            opener = text.find('/*')
            if opener >= 0 and text[:opener].strip() == '':
                inner = text[opener + 2:text.rfind('*/')]
                lines = [re.sub(r'^\s*\*+<?\s?', '', line) for line in inner.split('\n')]
        else:
            while index >= 0 and re.match(r'\s*//', before[index]):
                lines.insert(0, re.sub(r'^\s*//+[!<]?\s?', '', before[index]))
                index -= 1

    line_end = body.find('\n', end)
    trailing = body[end:line_end if line_end >= 0 else len(body)]
    match = re.match(r'\s*//+<?\s?(.*)$', trailing) or re.match(r'\s*/\*+<?\s?(.*?)\s*\*/', trailing)
    if match:
        lines.append('')
        lines.append(match.group(1))

    return _comment_text(lines)


def blank_comments(code: str) -> str:
    result = list(code)
    i = 0
    n = len(code)
    while i < n:
        if code.startswith('//', i):
            end = code.find('\n', i)
            end = n if end < 0 else end
        elif code.startswith('/*', i):
            end = code.find('*/', i + 2)
            end = n if end < 0 else end + 2
        elif code[i] in '"\'':
            j = i + 1
            while j < n and code[j] != code[i] and code[j] != '\n':
                j += 2 if code[j] == '\\' else 1
            i = j + 1
            continue
        else:
            i += 1
            continue
        for k in range(i, end):
            if result[k] != '\n':
                result[k] = ' '
        i = end
    return ''.join(result)


def find_properties(body: str, context: str):
    code = blank_comments(body)
    matches = list(PROPERTY_PATTERN.finditer(code))
    starts = {m.start() for m in matches}
    for marker in re.finditer(r'\bPROPERTY\s*\(', code):
        if marker.start() not in starts:
            line = code[:marker.start()].count('\n') + 1
            raise HeaderToolError(f"{context}: could not parse the declaration following PROPERTY(...) on line {line} of its body")
    return matches


def _parse_number(text: str, context: str) -> float:
    literal = text.strip()
    if re.search(r'[.eE]', literal):
        literal = re.sub(r'[fF]$', '', literal)
    try:
        value = float(literal)
    except ValueError:
        raise HeaderToolError(f"{context}: Range bound '{text.strip()}' is not a number")
    if math.isnan(value) or math.isinf(value):
        raise HeaderToolError(f"{context}: Range bound '{text.strip()}' must be finite")
    return value


def parse_specifiers(text: str, context: str, known: list[str] = None, macro: str = "PROPERTY") -> dict:
    known = KNOWN_SPECIFIERS if known is None else known
    specifiers = {}
    for part in split_top_level(text):
        name, has_value, value = part.partition('=')
        name = name.strip()
        if not re.fullmatch(r'\w+', name):
            raise HeaderToolError(f"{context}: malformed {macro} specifier '{part}'")
        if name not in known:
            raise HeaderToolError(f"{context}: unknown {macro} specifier '{name}' (known: {', '.join(known)})")
        if name in specifiers:
            raise HeaderToolError(f"{context}: {macro} specifier '{name}' is given twice")
        specifiers[name] = value.strip() if has_value else None
    return specifiers


def parse_range(value: str, context: str) -> tuple[float, float]:
    match = re.fullmatch(r'\(\s*([^,()]+?)\s*,\s*([^,()]+?)\s*\)', value or '')
    if not match:
        raise HeaderToolError(f"{context}: Range must be written as Range = (Min, Max)")
    low = _parse_number(match.group(1), context)
    high = _parse_number(match.group(2), context)
    if low >= high:
        raise HeaderToolError(f"{context}: Range minimum {match.group(1)} must be below its maximum {match.group(2)}")
    return low, high


def _cpp_double(value: float) -> str:
    return repr(float(value))


def parse_name_list(value: str, context: str) -> list[str]:
    match = re.fullmatch(r'\(\s*(.*?)\s*\)', value or '')
    names = [name.strip() for name in match.group(1).split(',')] if match and match.group(1).strip() else []
    if not match or any(not re.fullmatch(r'[A-Za-z_]\w*', name) for name in names):
        raise HeaderToolError(f"{context}: Params must be written as Params = (Name, ...)")
    return names


def _is_object_class(name: str) -> bool:
    from HeaderTool.Class import Class
    return name == "Object" or any(c.Name == name for c in Class.ALL_CLASSES)


class SignatureParameter:
    def __init__(self, name: str, storage: str, is_pointer: bool, is_output: bool, default: str = None):
        self.Name = name
        self.Storage = storage
        self.IsPointer = is_pointer
        self.IsOutput = is_output
        self.Default = default

    def argument(self) -> str:
        return f"params.{self.Name}.Get()" if self.IsPointer else f"params.{self.Name}"


def parameter_storage(cpp_type: str, context: str) -> tuple[str, bool, bool]:
    typename = normalize_type(cpp_type)
    is_const = typename.startswith('const ')
    if is_const:
        typename = typename[len('const '):]
    is_reference = typename.endswith('&')
    if is_reference:
        typename = typename[:-1]
    if typename.endswith('*'):
        target = typename[:-1]
        if target.startswith('const '):
            target = target[len('const '):]
        if not _is_object_class(target):
            raise HeaderToolError(f"{context}: '{cpp_type}' is not a pointer to an ARTIFACT_CLASS type")
        return f"WeakObjectPtr<{target}>", True, is_reference and not is_const
    return typename, False, is_reference and not is_const


def display_name_of(storage: str) -> str:
    base = storage
    for template in ("WeakObjectPtr", "SharedObjectPtr", "SubclassOf", "Array", "Map"):
        if _is_template(base, template):
            base = split_top_level(_template_argument(base, template))[-1]
    base = re.sub(r'_t$', '', base.split('::')[-1])
    return base[:1].upper() + base[1:]


def generate_signature(struct_name: str, function_var: str, function_name: str, owner_name: str,
                       parameters: list[SignatureParameter], context: str) -> list[str]:
    members = " ".join(f"{p.Storage} {p.Name}{'' if p.Default is None else ' = ' + p.Default};" for p in parameters)
    lines = [f"struct {struct_name} {{ {members} }};"]
    inputs = []
    outputs = []
    for parameter in parameters:
        generated = generate_property_type(normalize_type(parameter.Storage), parameter.Name, struct_name, True, context,
                                           var_name=f"{function_var}_{parameter.Name}")
        lines.append(generated.Code)
        (outputs if parameter.IsOutput else inputs).append(f"&{generated.Var}")
    lines += [
        f'static ::Function {function_var} = ::Function("{function_name}", "{owner_name}");',
        f"{function_var}.Inputs = {{ {', '.join(inputs)} }};",
        f"{function_var}.Outputs = {{ {', '.join(outputs)} }};",
        f"{function_var}.NewParams = []() -> void* {{ return new {struct_name}(); }};",
        f"{function_var}.DeleteParams = [](void* data) {{ delete reinterpret_cast<{struct_name}*>(data); }};",
        f"{function_var}.CopyParams = [](void* to, const void* from) {{ *reinterpret_cast<{struct_name}*>(to) = *reinterpret_cast<const {struct_name}*>(from); }};",
    ]
    return lines


def generate_property_type(full_typename: str, prop_name: str, class_or_struct_typename: str, use_offset: bool = True,
                           context: str = "", var_name: str = None, delegate_params: list[str] = None) -> GeneratedProperty:
    offset = f"offsetof({class_or_struct_typename}, {prop_name})" if use_offset else "0"
    var = var_name or f"_Property_{prop_name}"
    context = context or f"{class_or_struct_typename}::{prop_name}"

    def simple(kind: str, property_type: str, initializer: str = "", element: GeneratedProperty = None,
               is_unsigned: bool = False) -> GeneratedProperty:
        code = SIMPLE_PROPERTY.format(PROPERTY_TYPE=property_type, VAR=var, PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=initializer)
        if element:
            code = element.Code + "\n" + code
        return GeneratedProperty(kind, var, code, element, is_unsigned)

    if full_typename in INT_TYPES:
        is_unsigned, bits = INT_TYPES[full_typename]
        return simple("Int", "IntProperty", f", {'true' if is_unsigned else 'false'}, {bits}", is_unsigned=is_unsigned)

    if full_typename == "bool":
        return simple("Bool", "BoolProperty")

    if full_typename == "float":
        return simple("Float", "FloatProperty", ", false")
    if full_typename == "double":
        return simple("Float", "FloatProperty", ", true")

    if full_typename == "String":
        return simple("String", "StringProperty")

    if _is_template(full_typename, "SharedObjectPtr"):
        inner_type = _template_argument(full_typename, "SharedObjectPtr")
        return simple("Object", "SharedObjectPtrProperty", f', Class("{inner_type}")')

    if _is_template(full_typename, "WeakObjectPtr"):
        inner_type = _template_argument(full_typename, "WeakObjectPtr")
        return simple("Object", "WeakObjectPtrProperty", f', Class("{inner_type}")')

    if _is_template(full_typename, "SubclassOf"):
        inner_type = _template_argument(full_typename, "SubclassOf")
        return simple("SubclassOf", "SubclassOfProperty", f', Class("{inner_type}")')

    if full_typename == "Class":
        return simple("SubclassOf", "SubclassOfProperty", ', Class("Object")')

    if _is_template(full_typename, "Delegate"):
        argument_types = split_top_level(_template_argument(full_typename, "Delegate"))
        names = delegate_params if delegate_params is not None else [None] * len(argument_types)
        if len(names) != len(argument_types):
            raise HeaderToolError(f"{context}: Params names {len(names)} arguments, but '{full_typename}' has {len(argument_types)}")
        parameters = []
        for index, argument_type in enumerate(argument_types):
            storage, is_pointer, _ = parameter_storage(argument_type, context)
            name = names[index] or display_name_of(storage)
            if any(existing.Name == name for existing in parameters):
                name = f"{name}{index + 1}"
            parameters.append(SignatureParameter(name, storage, is_pointer, True))
        struct_name = f"{var}_Params"
        signature_var = f"{var}_Signature"
        signature_code = generate_signature(struct_name, signature_var, prop_name, class_or_struct_typename, parameters, context)
        lambdas = DELEGATE_LAMBDAS.format(
            DELEGATE_TYPE=full_typename,
            PARAMS_STRUCT=struct_name,
            ARGUMENTS=", ".join(f"{argument_type} a{index}" for index, argument_type in enumerate(argument_types)),
            ASSIGNMENTS="\n".join(f"        params.{parameter.Name} = a{index};" for index, parameter in enumerate(parameters)))
        signature = ', '.join(argument_types)
        generated = simple("Delegate", "DelegateProperty", f', {cpp_string(signature)}, &{signature_var},{lambdas}')
        generated.Code = "\n".join(signature_code) + "\n" + generated.Code
        return generated

    if _is_template(full_typename, "Array"):
        inner_type = _template_argument(full_typename, "Array")
        inner = generate_property_type(inner_type, prop_name + "_InnerArrayProperty", class_or_struct_typename, False, context,
                                       var_name=f"{var}_InnerArrayProperty")
        return simple("Array", "ArrayProperty", f', &{inner.Var},{ARRAY_LAMBDAS.format(PROPERTY_TYPE=inner_type)}', inner)

    if _is_template(full_typename, "Map"):
        arguments = split_top_level(_template_argument(full_typename, "Map"))
        if len(arguments) != 2:
            raise HeaderToolError(f"{context}: '{full_typename}' needs exactly a key and a value type")
        key_type, value_type = arguments
        key = generate_property_type(key_type, prop_name + "_MapKeyProperty", class_or_struct_typename, False, context,
                                     var_name=f"{var}_MapKeyProperty")
        if key.Kind not in MAP_KEY_KINDS:
            raise HeaderToolError(f"{context}: '{key_type}' cannot be a Map key; use String, an integer, bool, an enum, UUID, Class or SubclassOf<T>")
        value = generate_property_type(value_type, prop_name + "_MapValueProperty", class_or_struct_typename, False, context,
                                       var_name=f"{var}_MapValueProperty")
        lambdas = MAP_LAMBDAS.format(MAP_TYPE=full_typename, KEY_TYPE=key_type, VALUE_TYPE=value_type)
        code = key.Code + "\n" + value.Code + "\n" + SIMPLE_PROPERTY.format(
            PROPERTY_TYPE="MapProperty", VAR=var, PROPERTY_NAME=prop_name, OFFSET=offset,
            INITIALIZER=f', &{key.Var}, &{value.Var},{lambdas}')
        return GeneratedProperty("Map", var, code, value)

    if full_typename == "UUID":
        return simple("UUID", "UUIDProperty")

    if full_typename in BUILTIN_STRUCTS:
        return simple("Struct", "StructProperty", f', "{full_typename}"')

    from HeaderTool.HeaderTool import Struct, Enum
    for struct in Struct.ALL_STRUCTS:
        if struct.Name == full_typename:
            return simple("Struct", "StructProperty", f', "{full_typename}"')

    for enum in Enum.ALL_ENUMS:
        if enum.Name == full_typename:
            byte_size = _enum_underlying_byte_size(enum.UnderlyingType)
            return simple("Enum", "EnumProperty", f', "{full_typename}", {byte_size}')

    raise HeaderToolError(f"{context}: property type '{full_typename}' is not supported")


def _apply_specifiers(generated: GeneratedProperty, specifiers: dict, context: str) -> list[str]:
    statements = []
    if "Range" in specifiers:
        low, high = parse_range(specifiers["Range"], context)
        target = generated.numeric_target()
        if target is None:
            raise HeaderToolError(f"{context}: Range only applies to integer and float properties (or containers of them)")
        if target.Kind == "Int" and (not low.is_integer() or not high.is_integer()):
            raise HeaderToolError(f"{context}: Range of an integer property needs whole-number bounds")
        if target.IsUnsigned and low < 0:
            raise HeaderToolError(f"{context}: Range of an unsigned property cannot go below 0")
        statements.append(f"{target.Var}.SetRange({_cpp_double(low)}, {_cpp_double(high)});")
    return statements


def generate_properties_registration_code(typename: str, body: str) -> str:
    raw_properties = []
    for m in find_properties(body, typename):
        type_name = normalize_type(m.group('type'))
        prop_name = m.group('name').strip()
        context = f"{typename}::{prop_name}"
        specifiers = parse_specifiers(m.group('specifiers'), context)
        comment = extract_comment(body, m.start(), m.end())
        raw_properties.append((type_name, prop_name, specifiers, comment, context))

    create_properties_list = []
    for type_name, prop_name, specifiers, comment, context in raw_properties:
        delegate_params = None
        if "Params" in specifiers:
            if not _is_template(type_name, "Delegate"):
                raise HeaderToolError(f"{context}: Params only applies to Delegate properties")
            delegate_params = parse_name_list(specifiers["Params"], context)
        generated = generate_property_type(type_name, prop_name, typename, context=context, delegate_params=delegate_params)
        lines = [generated.Code] + _apply_specifiers(generated, specifiers, context)
        if comment:
            lines.append(f"{generated.Var}.SetComment({cpp_string(comment)});")
        create_properties_list.append("\n".join(lines))

    create_properties = "\n".join(create_properties_list)

    properties_list = "\n                ".join(
        f"&_Property_{prop_name},"
        for _, prop_name, _, _, _ in raw_properties
    )

    indented_properties = "\n".join(
        "            " + line
        for line in create_properties.splitlines()
    )

    return f"""struct InternalRegisterProperties {{
        InternalRegisterProperties() {{
{indented_properties}
            Property::RegisterTypeProperties("{typename}", {{
                {properties_list}
            }});
        }}
    }};
    inline static InternalRegisterProperties _InternalRegisterProperties;
"""
