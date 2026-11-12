import json
import os
from HeaderTool.Class import Class
from HeaderTool.Enum import Enum
from HeaderTool.Functions import find_functions, _parse_parameters
from HeaderTool.Properties import HeaderToolError, parameter_storage, _is_template, _template_argument
from SDK.Util import smart_open

INT_TYPES = {
    "int8_t": "sbyte", "uint8_t": "byte", "int16_t": "short", "uint16_t": "ushort",
    "int32_t": "int", "uint32_t": "uint", "int64_t": "long", "uint64_t": "ulong",
    "int8": "sbyte", "uint8": "byte", "int16": "short", "uint16": "ushort",
    "int32": "int", "uint32": "uint", "int64": "long", "uint64": "ulong",
}
BLITTABLE = {"float": "float", "double": "double", "Vec2": "Vec2", "Vec3": "Vec3", "Vec4": "Vec4", "Color": "Color", "Quat": "Quat"}
RESERVED_MEMBERS = {"Retain", "Release", "IsValid", "Handle", "Equals", "GetType", "ToString", "GetHashCode", "FromHandle"}
CSHARP_KEYWORDS = {
    "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char", "checked", "class", "const", "continue",
    "decimal", "default", "delegate", "do", "double", "else", "enum", "event", "explicit", "extern", "false", "finally",
    "fixed", "float", "for", "foreach", "goto", "if", "implicit", "in", "int", "interface", "internal", "is", "lock",
    "long", "namespace", "new", "null", "object", "operator", "out", "override", "params", "private", "protected",
    "public", "readonly", "ref", "return", "sbyte", "sealed", "short", "sizeof", "stackalloc", "static", "string",
    "struct", "switch", "this", "throw", "true", "try", "typeof", "uint", "ulong", "unchecked", "unsafe", "ushort",
    "using", "virtual", "void", "volatile", "while",
}
ENUM_UNDERLYING = {"uint8_t": "byte", "int8_t": "sbyte", "uint16_t": "ushort", "int16_t": "short",
                   "uint32_t": "uint", "int32_t": "int", "uint64_t": "ulong", "int64_t": "long"}


def _identifier(name: str) -> str:
    return f"@{name}" if name in CSHARP_KEYWORDS else name


class _Marshal:
    def __init__(self, csharp: str, slot: str, to_slot: str, from_slot: str):
        self.CSharp = csharp
        self.Slot = slot
        self.ToSlot = to_slot
        self.FromSlot = from_slot


def _marshal(storage: str, classes: set, enums: set) -> _Marshal:
    if storage in INT_TYPES:
        return _Marshal(INT_TYPES[storage], INT_TYPES[storage], "{0}", "{0}")
    if storage in BLITTABLE:
        return _Marshal(BLITTABLE[storage], BLITTABLE[storage], "{0}", "{0}")
    if storage in enums:
        return _Marshal(storage, storage, "{0}", "{0}")
    if storage == "bool":
        return _Marshal("bool", "byte", "(byte)({0} ? 1 : 0)", "{0} != 0")
    if storage == "String":
        return _Marshal("string", "IntPtr", "{0}", "Native.ReadString({0})")
    for template in ("WeakObjectPtr", "SharedObjectPtr"):
        if _is_template(storage, template):
            target = _template_argument(storage, template)
            target = target if target in classes else "Object"
            return _Marshal(target, "IntPtr", "{0}?.Handle ?? IntPtr.Zero", "Object.FromHandle<" + target + ">({0})")
    return None


def _method(cls: Class, match, body: str, classes: set, enums: set, index: int) -> list[str]:
    name = match.group('name')
    if name in RESERVED_MEMBERS:
        return []
    context = f"{cls.Name}::{name}"
    try:
        parameters = _parse_parameters(match.group('params'), context)
        return_type = match.group('ret').strip()
        result = None if return_type == "void" else _marshal(parameter_storage(return_type, context)[0], classes, enums)
    except HeaderToolError:
        return []
    marshals = [_marshal(p.Storage, classes, enums) for p in parameters]
    if any(m is None for m in marshals) or (return_type != "void" and result is None):
        return []

    is_static = 'static' in match.group('prefix').split()
    signature = ", ".join(f"{'out ' if p.IsOutput else ''}{m.CSharp} {_identifier(p.Name)}" for p, m in zip(parameters, marshals))
    returns = result.CSharp if result else "void"
    field = f"s_Function{index}"

    lines = [f"        static IntPtr {field};",
             f"        public {'static ' if is_static else ''}{returns} {name}({signature})",
             "        {"]
    ordered = [(p, m) for p, m in zip(parameters, marshals) if not p.IsOutput] + \
              [(p, m) for p, m in zip(parameters, marshals) if p.IsOutput]
    for slot, (parameter, marshal) in enumerate(ordered):
        if parameter.IsOutput:
            lines.append(f"            {marshal.Slot} a{slot} = default;")
        elif marshal.CSharp == "string":
            lines.append(f"            using Utf8 u{slot} = new Utf8({_identifier(parameter.Name)});")
            lines.append(f"            IntPtr a{slot} = u{slot}.Pointer;")
        else:
            lines.append(f"            {marshal.Slot} a{slot} = {marshal.ToSlot.format(_identifier(parameter.Name))};")
    arguments = "null"
    if ordered:
        lines.append(f"            void** args = stackalloc void*[{len(ordered)}];")
        lines += [f"            args[{slot}] = &a{slot};" for slot in range(len(ordered))]
        arguments = "args"
    if result:
        lines.append(f"            {result.Slot} result = default;")
    self_handle = "IntPtr.Zero" if is_static else "Handle"
    lines.append(f'            Native.Call(ref {field}, "{cls.Name}", "{name}", {self_handle}, {arguments}, {"&result" if result else "null"});')
    for slot, (parameter, marshal) in enumerate(ordered):
        if parameter.IsOutput:
            lines.append(f"            {_identifier(parameter.Name)} = {marshal.FromSlot.format(f'a{slot}')};")
    if result:
        lines.append(f"            return {result.FromSlot.format('result')};")
    lines.append("        }")
    return lines


def generate_csharp_bindings(header_tool, output_dir: str):
    excluded_modules = header_tool.editor_only_modules
    header_of = {}
    for header, types in header_tool.types_per_header.items():
        for type_ in types:
            header_of[type_.Name] = header_tool.header_paths[header]

    all_classes = {c.Name: c for c in Class.ALL_CLASSES if c.ModuleName not in excluded_modules}
    bound = {}

    def reaches_object(name: str, seen: set) -> bool:
        if name == "Object":
            return True
        cls = all_classes.get(name)
        if cls is None or name in seen or cls.is_a("Property"):
            return False
        return reaches_object(cls.ParentClassName, seen | {name})

    for name, cls in all_classes.items():
        if name != "Object" and reaches_object(name, set()):
            bound[name] = cls

    enums = {e.Name: e for e in Enum.ALL_ENUMS if header_of.get(e.Name) and _module_of_header(header_of[e.Name], header_tool) not in excluded_modules}
    class_names = set(bound.keys())
    enum_names = set(enums.keys())

    lines = ["using System;", "", "namespace Artifact", "{"]
    for enum in sorted(enums.values(), key=lambda e: e.Name):
        underlying = ENUM_UNDERLYING.get(enum.UnderlyingType or "int32_t", "int")
        values = ", ".join(f"{_identifier(n)} = {v}" for n, v in enum.parse_enum_values())
        lines.append(f"    public enum {enum.Name} : {underlying} {{ {values} }}")
    lines.append("")

    function_index = 0
    for cls in sorted(bound.values(), key=lambda c: c.Name):
        lines.append(f"    public unsafe partial class {cls.Name} : {cls.ParentClassName}")
        lines.append("    {")
        for match in find_functions(cls.Body, cls.Name):
            method = _method(cls, match, cls.Body, class_names, enum_names, function_index)
            if method:
                lines += method
                function_index += 1
        lines.append("    }")
        lines.append("")
    lines.append("}")

    os.makedirs(output_dir, exist_ok=True)
    with smart_open(f"{output_dir}/Bindings.g.cs") as file:
        file.write("\n".join(lines) + "\n")

    reflection = {
        "Classes": {name: {"Parent": cls.ParentClassName, "Module": cls.ModuleName, "Header": header_of.get(name, "")}
                    for name, cls in all_classes.items()},
        "Enums": {name: {"Header": header_of.get(name, ""), "Underlying": enum.UnderlyingType or "int32_t"}
                  for name, enum in enums.items()},
    }
    with smart_open(f"{output_dir}/Reflection.json") as file:
        file.write(json.dumps(reflection, indent=2, sort_keys=True) + "\n")


def _module_of_header(header_path: str, header_tool) -> str:
    for module, headers in header_tool.headers_per_module.items():
        if header_path in headers:
            return module
    return ""
