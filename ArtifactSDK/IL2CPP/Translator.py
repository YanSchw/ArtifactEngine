import math
import re
import struct
from IL2CPP.IL import decode
from SDK.Job import JobError


class TranspileError(JobError):
    pass


VALUE_STRUCTS = {"Artifact.Vec2": "Vec2", "Artifact.Vec3": "Vec3", "Artifact.Vec4": "Vec4", "Artifact.Color": "Color", "Artifact.Quat": "Quat"}
STRUCT_MEMBERS = {"X": "x", "Y": "y", "Z": "z", "W": "w", "R": "r", "G": "g", "B": "b", "A": "a"}
STRUCT_ZERO = {"Vec2": "Vec2(0.0f)", "Vec3": "Vec3(0.0f)", "Vec4": "Vec4(0.0f)", "Color": "Color(0.0f)", "Quat": "Quat(0.0f, 0.0f, 0.0f, 0.0f)"}
STRUCT_OPERATORS = {"op_Addition": "+", "op_Subtraction": "-", "op_Multiply": "*", "op_Division": "/"}
PRIMITIVES = {"bool": ("bool", "bool"), "char": ("int", "char16_t"), "i1": ("int", "int8_t"), "u1": ("int", "uint8_t"),
              "i2": ("int", "int16_t"), "u2": ("int", "uint16_t"), "i4": ("int", "int32_t"), "u4": ("int", "uint32_t"),
              "i8": ("int", "int64_t"), "u8": ("int", "uint64_t"), "r4": ("float", "float"), "r8": ("float", "double"),
              "string": ("string", "String")}
CONVERSIONS = {"i1": "int8_t", "i2": "int16_t", "i4": "int32_t", "i8": "int64_t", "u1": "uint8_t", "u2": "uint16_t",
               "u4": "uint32_t", "u8": "uint64_t", "r4": "float", "r8": "double", "i": "intptr_t", "u": "uintptr_t", "r.un": "double"}
UNSIGNED = {"int8_t": "uint8_t", "int16_t": "uint16_t", "int32_t": "uint32_t", "int64_t": "uint64_t"}
SYSTEM_VALUE_TYPES = {"System.Boolean", "System.Byte", "System.SByte", "System.Int16", "System.UInt16", "System.Int32",
                      "System.UInt32", "System.Int64", "System.UInt64", "System.Single", "System.Double"}
MATH = {"Abs": "std::abs", "Sqrt": "std::sqrt", "Sin": "std::sin", "Cos": "std::cos", "Tan": "std::tan", "Asin": "std::asin",
        "Acos": "std::acos", "Atan": "std::atan", "Atan2": "std::atan2", "Pow": "std::pow", "Exp": "std::exp",
        "Floor": "std::floor", "Ceiling": "std::ceil", "Round": "std::nearbyint", "Min": "std::min", "Max": "std::max",
        "Clamp": "std::clamp"}
HANDLER = "System.Runtime.CompilerServices.DefaultInterpolatedStringHandler"
LIFECYCLE = {"BeginPlay", "EndPlay", "WorldUpdate"}
BRANCHES = {"br", "brfalse", "brtrue", "beq", "bge", "bgt", "ble", "blt", "bne.un", "bge.un", "bgt.un", "ble.un", "blt.un"}
COMPARISONS = {"beq": "==", "bne.un": "!=", "bge": ">=", "bgt": ">", "ble": "<=", "blt": "<",
               "ceq": "==", "cgt": ">", "clt": "<"}
UNORDERED = {"bge.un": "<", "bgt.un": "<=", "ble.un": ">", "blt.un": ">=", "cgt.un": "<=", "clt.un": ">="}
BINARY = {"add": "+", "sub": "-", "mul": "*", "div": "/", "rem": "%", "and": "&", "or": "|", "xor": "^", "shl": "<<", "shr": ">>"}
UNSUPPORTED = {
    "box": "boxing is not supported", "unbox": "boxing is not supported", "unbox.any": "boxing is not supported",
    "throw": "exceptions are not supported", "rethrow": "exceptions are not supported",
    "endfinally": "exceptions are not supported", "ldftn": "delegates and lambdas are not supported",
    "ldvirtftn": "delegates and lambdas are not supported", "ldlen": "arrays are not supported",
    "ldelema": "arrays are not supported", "ldelem": "arrays are not supported", "stelem": "arrays are not supported",
}
CPP_KEYWORDS = {"alignas", "alignof", "and", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char",
                "class", "compl", "concept", "const", "consteval", "constexpr", "continue", "decltype", "default",
                "delete", "do", "double", "else", "enum", "explicit", "export", "extern", "false", "float", "for",
                "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not",
                "nullptr", "operator", "or", "private", "protected", "public", "register", "requires", "return",
                "short", "signed", "sizeof", "static", "struct", "switch", "template", "this", "throw", "true", "try",
                "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "while",
                "xor", "Super"}


def identifier(name: str) -> str:
    backing = re.fullmatch(r"<(\w+)>k__BackingField", name)
    if backing:
        return f"{backing.group(1)}_BackingField"
    name = re.sub(r"\W", "_", name)
    if name[:1].isdigit():
        name = "_" + name
    return name + "_" if name in CPP_KEYWORDS else name


def unwrap(expr: str) -> str:
    if not (expr.startswith("(") and expr.endswith(")")):
        return expr
    depth = 0
    for i, char in enumerate(expr):
        depth += {"(": 1, ")": -1}.get(char, 0)
        if depth == 0 and i < len(expr) - 1:
            return expr
    return expr[1:-1]


def string_literal(text: str) -> str:
    escaped = []
    for byte in text.encode("utf-8"):
        char = chr(byte)
        if char in '"\\':
            escaped.append("\\" + char)
        elif 0x20 <= byte < 0x7F:
            escaped.append(char)
        else:
            escaped.append(f"\\{byte:03o}")
    return f'String("{"".join(escaped)}")'


def float_literal(value: float, single: bool) -> str:
    cpp = "float" if single else "double"
    if math.isnan(value):
        return f"std::numeric_limits<{cpp}>::quiet_NaN()"
    if math.isinf(value):
        return f"{'-' if value < 0 else ''}std::numeric_limits<{cpp}>::infinity()"
    text = repr(value)
    if single:
        for precision in range(6, 10):
            text = f"%.{precision}g" % value
            if struct.unpack("<f", struct.pack("<f", float(text)))[0] == value:
                break
    if not any(c in text for c in ".en"):
        text += ".0"
    return text + ("f" if single else "")


class CType:
    def __init__(self, kind: str, cpp: str = "", inner=None):
        self.Kind = kind
        self.Cpp = cpp
        self.Inner = inner

    def decl(self, storage: bool) -> str:
        if self.Kind == "object":
            return f"WeakObjectPtr<{self.Cpp}>" if storage else f"{self.Cpp}*"
        if self.Kind == "byref":
            return f"{self.Inner.decl(True)}&"
        if self.Kind == "null":
            return "Object*"
        return self.Cpp

    def zero(self, storage: bool) -> str:
        if self.Kind == "bool":
            return "false"
        if self.Kind == "int":
            return "0"
        if self.Kind == "float":
            return "0.0f" if self.Cpp == "float" else "0.0"
        if self.Kind == "struct":
            return STRUCT_ZERO[self.Cpp]
        if self.Kind == "enum":
            return f"static_cast<{self.Cpp}>(0)"
        if self.Kind in ("object", "null") and not storage:
            return "nullptr"
        return ""


BOOL = CType("bool", "bool")
INT32 = CType("int", "int32_t")
INT64 = CType("int", "int64_t")
FLOAT = CType("float", "float")
DOUBLE = CType("float", "double")
STRING = CType("string", "String")
CLASS = CType("classref", "Class")
NULL = CType("null")
STRING_ARRAY = CType("strarray")


def byref(inner: CType) -> CType:
    return CType("byref", inner=inner)


class Value:
    def __init__(self, expr: str, type_: CType, const=False, weak=False, temp=False):
        self.Expr = expr
        self.Type = type_
        self.Const = const
        self.Weak = weak
        self.Temp = temp
        self.Parts = None

    def stable(self) -> bool:
        return self.Const or self.Temp or self.Expr == "this" or self.Type.Kind in ("byref", "strarray")


class Script:
    def __init__(self, module: str, assembly, type_def):
        self.Module = module
        self.Assembly = assembly
        self.Type = type_def
        self.Name = type_def.Name

    @property
    def Parent(self) -> str:
        return self.Assembly.type_name(self.Type.Extends)


class Program:
    def __init__(self, assemblies: dict, reflection: dict):
        self.Classes = reflection["Classes"]
        self.Enums = reflection["Enums"]
        self.Seen = set()
        definitions = {self.full_name(type_def): Script(module, assembly, type_def)
                       for module, assembly in assemblies.items()
                       for type_def in assembly.Tables["TypeDef"] if type_def.Name != "<Module>"}
        self.Scripts = {name: script for name, script in definitions.items() if self._reaches_object(script, definitions)}

    @staticmethod
    def full_name(type_def) -> str:
        return f"{type_def.Namespace}.{type_def.Name}" if type_def.Namespace else type_def.Name

    def _reaches_object(self, script: Script, definitions: dict) -> bool:
        parent = script.Parent
        if parent.startswith("Artifact."):
            return parent[9:] == "Object" or parent[9:] in self.Classes
        return parent in definitions and self._reaches_object(definitions[parent], definitions)

    def class_name(self, full_name: str):
        if full_name in self.Scripts:
            return self.Scripts[full_name].Name
        if full_name.startswith("Artifact.") and (full_name[9:] == "Object" or full_name[9:] in self.Classes):
            return full_name[9:]
        return None

    def ctype(self, sig) -> CType:
        if sig.Kind == "void":
            return None
        if sig.Kind in PRIMITIVES:
            return CType(*PRIMITIVES[sig.Kind])
        if sig.Kind == "byref":
            return byref(self.ctype(sig.Element))
        if sig.Kind in ("class", "valuetype"):
            if sig.Name in VALUE_STRUCTS:
                return CType("struct", VALUE_STRUCTS[sig.Name])
            if sig.Name == HANDLER:
                return STRING
            if sig.Name == "Artifact.Class":
                return CLASS
            if sig.Name == "System.Type":
                raise TranspileError("System.Type values are not supported; use Artifact.Class")
            if sig.Name.startswith("Artifact.") and sig.Name[9:] in self.Enums:
                self.Seen.add(("enum", sig.Name[9:]))
                return CType("enum", sig.Name[9:])
            name = self.class_name(sig.Name)
            if name:
                self.Seen.add(("class", name))
                return CType("object", name)
            raise TranspileError(f"type {sig.Name} is not supported")
        if sig.Kind == "object":
            raise TranspileError("System.Object values are not supported")
        if sig.Kind in ("generic", "var", "mvar"):
            raise TranspileError("generics are not supported")
        if sig.Kind == "szarray":
            raise TranspileError("arrays are not supported")
        raise TranspileError(f"type {sig!r} is not supported")

    def type_token_name(self, assembly, token: int) -> str:
        row = assembly.token(token)
        if row.Table == "TypeDef":
            return self.full_name(row)
        return assembly.type_name((row.Table, row.Index))

    def method_ref(self, assembly, token: int):
        row = assembly.token(token)
        generic = []
        if row.Table == "MethodSpec":
            generic = assembly.method_spec_args(row.Instantiation)
            row = assembly.row(*row.Method)
        if row.Table == "MethodDef":
            return self.full_name(assembly.declaring_type(row)), row.Name, assembly.method_sig(row.Signature), generic
        return assembly.type_name(row.Class), row.Name, assembly.method_sig(row.Signature), generic

    def field_ref(self, assembly, token: int):
        row = assembly.token(token)
        if row.Table == "Field":
            return self.full_name(assembly.field_owner(row)), row.Name, assembly.field_sig(row.Signature), bool(row.Flags & 0x10)
        owner = assembly.type_name(row.Class)
        script = self.Scripts.get(owner)
        definition = next((f for f in script.Assembly.fields_of(script.Type) if f.Name == row.Name), None) if script else None
        return owner, row.Name, assembly.field_sig(row.Signature), bool(definition and definition.Flags & 0x10)

    def overrides_in_script(self, script: Script, name: str) -> bool:
        current = self.Scripts.get(script.Parent)
        while current is not None:
            if any(m.Name == name and m.Flags & 0x40 for m in current.Assembly.methods_of(current.Type)):
                return True
            current = self.Scripts.get(current.Parent)
        return False


class MethodTranslator:
    def __init__(self, program: Program, script: Script, method, sig, params: list, is_ctor: bool):
        self.Program = program
        self.Assembly = script.Assembly
        self.Owner = script.Type
        self.Method = method
        self.Sig = sig
        self.Params = params
        self.IsCtor = is_ctor
        self.Where = f"{script.Name}.{method.Name}"
        self.Stack = []
        self.Lines = []
        self.Temps = {}
        self.Merges = {}
        self.Locals = []
        self.Constrained = None
        self.Unreachable = False

    def translate(self) -> list[str]:
        body = self.Assembly.method_body(self.Method)
        if body.HasExceptionHandlers:
            raise TranspileError(f"{self.Where}: exceptions are not supported")
        try:
            self.Locals = [self.Program.ctype(sig) for sig in self.Assembly.locals_sig(body.LocalsToken)]
        except TranspileError as error:
            raise TranspileError(f"{self.Where}: {error}")
        instructions = decode(body.Code)
        targets = set()
        for instruction in instructions:
            name = instruction.Name.removesuffix(".s")
            if name in BRANCHES or name == "leave":
                targets.add(instruction.Operand)
            elif name == "switch":
                targets.update(instruction.Operand)
        for instruction in instructions:
            if instruction.Offset in targets:
                self._enter_label(instruction.Offset)
            try:
                self._step(instruction)
            except TranspileError as error:
                raise TranspileError(f"{self.Where} IL_{instruction.Offset:04X} ({instruction.Name}): {error}")
        declarations = [self._declare(local.decl(True), f"l{i}", local.zero(True)) for i, local in enumerate(self.Locals)]
        declarations += [self._declare(type_.decl(False), name, type_.zero(False)) for name, type_ in self.Temps.items()]
        return declarations + self.Lines

    @staticmethod
    def _declare(cpp: str, name: str, zero: str) -> str:
        return f"{cpp} {name} = {zero};" if zero else f"{cpp} {name};"

    def _push(self, expr: str, type_: CType, const=False, weak=False, temp=False) -> Value:
        value = Value(expr, type_, const, weak, temp)
        self.Stack.append(value)
        return value

    def _pop(self) -> Value:
        if not self.Stack:
            raise TranspileError("evaluation stack underflow")
        return self.Stack.pop()

    def _temp(self, type_: CType, prefix: str = "t") -> str:
        name = f"{prefix}{len(self.Temps)}"
        self.Temps[name] = type_
        return name

    def _materialize(self, value: Value) -> Value:
        name = self._temp(value.Type)
        self.Lines.append(f"{name} = {self.coerce(value, value.Type)};")
        return Value(name, value.Type, temp=True)

    def _spill(self):
        for i, value in enumerate(self.Stack):
            if value.Parts is not None:
                value.Parts[:] = [part if part is None or part.stable() else self._materialize(part) for part in value.Parts]
            elif not value.stable():
                self.Stack[i] = self._materialize(value)

    def _statement(self, line: str):
        self._spill()
        self.Lines.append(line)

    def _call_result(self, expr: str, type_: CType):
        self._spill()
        if type_ is None:
            self.Lines.append(f"{expr};")
            return
        if type_.Kind == "byref":
            raise TranspileError("ref returns are not supported")
        name = self._temp(type_)
        self.Lines.append(f"{name} = {expr};")
        self._push(name, type_, temp=True)

    def _enter_label(self, offset: int):
        if self.Stack and not self.Unreachable:
            self._store_merge(offset)
        merge = self.Merges.get(offset)
        self.Stack = [Value(name, self.Temps[name], temp=True) for name in merge] if merge else []
        self.Lines.append(f"IL_{offset:04X}: ;")
        self.Unreachable = False

    def _store_merge(self, offset: int):
        merge = self.Merges.get(offset)
        if merge is None:
            merge = [self._temp(value.Type, "m") for value in self.Stack]
            self.Merges[offset] = merge
        for name, value in zip(merge, self.Stack):
            if self.Temps[name].Kind == "null":
                self.Temps[name] = value.Type
            self.Lines.append(f"{name} = {self.coerce(value, self.Temps[name])};")

    def raw(self, value: Value) -> str:
        if value.Type.Kind == "null":
            return "nullptr"
        return f"{value.Expr}.Get()" if value.Weak else value.Expr

    def coerce(self, value: Value, target: CType) -> str:
        source = value.Type
        if target.Kind == "object":
            return self.raw(value)
        if target.Kind == "null":
            return value.Expr
        if target.Kind == "string":
            return "String()" if source.Kind == "null" else value.Expr
        if target.Kind == "bool":
            if source.Kind == "object":
                return f"({self.raw(value)} != nullptr)"
            if source.Kind == "bool":
                return value.Expr
            if value.Const and value.Expr in ("0", "1"):
                return "true" if value.Expr == "1" else "false"
            return f"({value.Expr} != 0)"
        if target.Kind in ("int", "float", "enum") and source.Kind in ("int", "float", "bool", "enum") and source.Cpp != target.Cpp:
            return f"static_cast<{target.Cpp}>({value.Expr})"
        return value.Expr

    def _number(self, value: Value) -> str:
        return f"static_cast<int32_t>({value.Expr})" if value.Type.Kind == "enum" else value.Expr

    def _truthy(self, value: Value) -> str:
        if value.Type.Kind in ("object", "null"):
            return f"({self.raw(value)} != nullptr)"
        if value.Type.Kind == "string":
            return "true"
        return self._number(value)

    def _arithmetic_type(self, a: Value, b: Value) -> CType:
        types = [a.Type, b.Type]
        if any(t.Cpp == "double" for t in types):
            return DOUBLE
        if any(t.Kind == "float" for t in types):
            return FLOAT
        if a.Type.Kind == "int" and a.Type.Cpp == b.Type.Cpp:
            return a.Type
        return INT64 if any(t.Cpp in ("int64_t", "uint64_t") for t in types) else INT32

    def _compare(self, op: str, a: Value, b: Value) -> str:
        if {a.Type.Kind, b.Type.Kind} & {"object", "null"}:
            return f"({self.raw(a)} {'==' if op in ('beq', 'ceq') else '!='} {self.raw(b)})"
        left, right = self._number(a), self._number(b)
        if op not in UNORDERED:
            return f"({left} {COMPARISONS[op]} {right})"
        if "float" in (a.Type.Kind, b.Type.Kind):
            return f"!({left} {UNORDERED[op]} {right})"
        cpp = self._arithmetic_type(a, b).Cpp
        cast = UNSIGNED.get(cpp, cpp)
        return f"(static_cast<{cast}>({left}) {COMPARISONS[op.removesuffix('.un')]} static_cast<{cast}>({right}))"

    def _arg(self, index: int):
        if self.Sig.HasThis:
            if index == 0:
                return "this", CType("object", self.Owner.Name)
            index -= 1
        name, type_ = self.Params[index]
        return name, type_

    def _local(self, operand) -> int:
        if operand >= len(self.Locals):
            raise TranspileError(f"unknown local {operand}")
        return operand

    def _branch(self, target: int, condition: str = None):
        if self.Stack:
            self._store_merge(target)
        self.Lines.append(f"if ({unwrap(condition)}) goto IL_{target:04X};" if condition else f"goto IL_{target:04X};")

    def _step(self, instruction):
        name = instruction.Name
        operand = instruction.Operand
        base = name.removesuffix(".s")

        if name in ("nop", "break", "volatile.", "readonly.", "tail.", "unaligned."):
            return
        if name == "constrained.":
            self.Constrained = self.Program.type_token_name(self.Assembly, operand)
            return
        if name in UNSUPPORTED or name.startswith("ldelem") or name.startswith("stelem.") and name != "stelem.ref":
            raise TranspileError(UNSUPPORTED.get(name, "arrays are not supported"))

        if name.startswith("ldarg"):
            index = int(name[-1]) if name[-1].isdigit() else operand
            arg, type_ = self._arg(index)
            if base == "ldarga":
                self._push(arg, byref(type_))
            else:
                self._push(arg, type_, const=arg == "this")
        elif base in ("starg",):
            value = self._pop()
            arg, type_ = self._arg(operand)
            self._statement(f"{arg} = {self.coerce(value, type_)};")
        elif name.startswith("ldloc"):
            index = self._local(int(name[-1]) if name[-1].isdigit() else operand)
            local = self.Locals[index]
            if base == "ldloca":
                self._push(f"l{index}", byref(local))
            else:
                self._push(f"l{index}", local, weak=local.Kind == "object")
        elif name.startswith("stloc"):
            index = self._local(int(name[-1]) if name[-1].isdigit() else operand)
            value = self._pop()
            self._statement(f"l{index} = {self.coerce(value, self.Locals[index])};")
        elif name == "ldnull":
            self._push("nullptr", NULL, const=True)
        elif name == "ldtoken":
            full_name = self.Program.type_token_name(self.Assembly, operand)
            class_name = self.Program.class_name(full_name)
            if class_name is None:
                raise TranspileError(f"typeof({full_name}) is not supported")
            self.Program.Seen.add(("class", class_name))
            self._push(class_name, CType("typetoken", class_name), const=True)
        elif name.startswith("ldc.i4"):
            number = -1 if name == "ldc.i4.m1" else (operand if operand is not None else int(name[-1]))
            self._push("(-2147483647 - 1)" if number == -2147483648 else str(number), INT32, const=True)
        elif name == "ldc.i8":
            self._push("(-9223372036854775807LL - 1)" if operand == -(1 << 63) else f"{operand}LL", INT64, const=True)
        elif name in ("ldc.r4", "ldc.r8"):
            self._push(float_literal(operand, name == "ldc.r4"), FLOAT if name == "ldc.r4" else DOUBLE, const=True)
        elif name == "ldstr":
            self._push(string_literal(self.Assembly.user_string(operand & 0xFFFFFF)), STRING, const=True)
        elif name == "dup":
            value = self._pop()
            if not value.stable():
                value = self._materialize(value)
            self.Stack += [value, value]
        elif name == "pop":
            self._pop()
        elif name == "ret":
            if self.Sig.Return.Kind == "void":
                self.Lines.append("return;")
            else:
                self.Lines.append(f"return {self.coerce(self._pop(), self.Program.ctype(self.Sig.Return))};")
            self.Stack = []
            self.Unreachable = True
        elif base in ("br", "leave"):
            self._branch(operand)
            self.Stack = []
            self.Unreachable = True
        elif base in ("brtrue", "brfalse"):
            condition = self._truthy(self._pop())
            self._branch(operand, condition if base == "brtrue" else f"!{condition}")
        elif base in BRANCHES:
            b, a = self._pop(), self._pop()
            self._branch(operand, self._compare(base, a, b))
        elif name == "switch":
            value = self._pop()
            for target in operand:
                if self.Stack:
                    self._store_merge(target)
            cases = " ".join(f"case {i}: goto IL_{target:04X};" for i, target in enumerate(operand))
            self.Lines.append(f"switch ({self._number(value)}) {{ {cases} default: break; }}")
        elif name.startswith("ldind.") or name == "ldobj":
            address = self._pop()
            if address.Type.Kind != "byref":
                raise TranspileError("expected an address")
            self._push(address.Expr, address.Type.Inner, weak=address.Type.Inner.Kind == "object")
        elif name.startswith("stind.") or name == "stobj":
            value, address = self._pop(), self._pop()
            self._statement(f"{address.Expr} = {self.coerce(value, address.Type.Inner)};")
        elif name == "initobj":
            address = self._pop()
            inner = address.Type.Inner
            self._statement(f"{address.Expr} = {inner.zero(True) or inner.Cpp + '()'};")
        elif name in BINARY or name in ("div.un", "rem.un", "shr.un"):
            self._binary(name)
        elif name in ("neg", "not"):
            value = self._pop()
            self._push(f"({'-' if name == 'neg' else '~'}{self._number(value)})", value.Type if value.Type.Kind in ("int", "float") else INT32)
        elif name.startswith("conv."):
            suffix = name.removeprefix("conv.").removeprefix("ovf.").removesuffix(".un") if name != "conv.r.un" else "r.un"
            cpp = CONVERSIONS[suffix]
            value = self._pop()
            type_ = CType("float" if cpp in ("float", "double") else "int", cpp)
            self._push(f"static_cast<{cpp}>({self._number(value)})", type_)
        elif name in ("ceq", "cgt", "clt", "cgt.un", "clt.un"):
            b, a = self._pop(), self._pop()
            self._push(self._compare(name, a, b), BOOL)
        elif name in ("call", "callvirt"):
            self._call(operand, name == "callvirt")
        elif name == "newobj":
            self._new_object(operand)
        elif name in ("ldfld", "ldflda", "stfld", "ldsfld", "ldsflda", "stsfld"):
            self._field(name, operand)
        elif name in ("isinst", "castclass"):
            self._cast(operand)
        elif name == "newarr":
            self._new_string_array(operand)
        elif name == "stelem.ref":
            value, index, array = self._pop(), self._pop(), self._pop()
            if array.Parts is None or not index.Const:
                raise TranspileError("arrays are not supported")
            array.Parts[int(index.Expr)] = value
        else:
            raise TranspileError("instruction is not supported")

    def _binary(self, name: str):
        b, a = self._pop(), self._pop()
        type_ = a.Type if name.startswith("sh") and a.Type.Kind == "int" else self._arithmetic_type(a, b)
        left, right = self._number(a), self._number(b)
        if name.endswith(".un"):
            cast = UNSIGNED.get(type_.Cpp, type_.Cpp)
            symbol = {"div.un": "/", "rem.un": "%", "shr.un": ">>"}[name]
            self._push(f"static_cast<{type_.Cpp}>(static_cast<{cast}>({left}) {symbol} static_cast<{cast}>({right}))", type_)
        elif name == "rem" and type_.Kind == "float":
            self._push(f"std::fmod({left}, {right})", type_)
        else:
            self._push(f"({left} {BINARY[name]} {right})", type_)

    def _cast(self, token: int):
        value = self._pop()
        name = self.Program.class_name(self.Program.type_token_name(self.Assembly, token))
        if name is None:
            raise TranspileError(f"cannot cast to {self.Program.type_token_name(self.Assembly, token)}")
        self.Program.Seen.add(("class", name))
        type_ = CType("object", name)
        if value.Type.Kind == "null":
            self._push("nullptr", NULL, const=True)
            return
        self._push(f"Cast<{name}>({self.raw(value)})", type_)

    def _new_string_array(self, token: int):
        length = self._pop()
        if self.Program.type_token_name(self.Assembly, token) != "System.String" or not length.Const:
            raise TranspileError("arrays are not supported")
        array = self._push("", STRING_ARRAY)
        array.Parts = [None] * int(length.Expr)

    def _new_object(self, token: int):
        declaring, name, sig, _ = self.Program.method_ref(self.Assembly, token)
        self._check_owner(declaring)
        args = [self._pop() for _ in sig.Params][::-1]
        if declaring in VALUE_STRUCTS:
            parts = ", ".join(self.coerce(arg, self.Program.ctype(p)) for arg, p in zip(args, sig.Params))
            self._push(f"{VALUE_STRUCTS[declaring]}({parts})", CType("struct", VALUE_STRUCTS[declaring]))
        elif declaring == HANDLER:
            self._push("String()", STRING)
        elif self.Program.class_name(declaring):
            if sig.Params:
                raise TranspileError("constructors with parameters are not supported")
            class_name = self.Program.class_name(declaring)
            self.Program.Seen.add(("class", class_name))
            self._call_result(f"IL2CPP::New<{class_name}>()", CType("object", class_name))
        else:
            raise TranspileError(f"cannot create {declaring}")

    @staticmethod
    def _check_owner(name: str):
        if "`" in name:
            raise TranspileError(f"generic type {name.split('`')[0]} is not supported")
        if "<" in name:
            raise TranspileError("lambdas, closures and iterators are not supported")

    def _field(self, op: str, token: int):
        owner, name, sig, is_static = self.Program.field_ref(self.Assembly, token)
        self._check_owner(owner)
        value = self._pop() if op in ("stfld", "stsfld") else None
        target = self._pop() if op in ("ldfld", "ldflda", "stfld") else None
        if owner in VALUE_STRUCTS:
            expr, type_ = f"{target.Expr}.{STRUCT_MEMBERS[name]}", FLOAT
        elif owner == "Artifact.Class" and name == "None" and op in ("ldsfld", "ldsflda"):
            expr, type_ = "Class::None", CLASS
        elif owner in self.Program.Scripts:
            type_ = self.Program.ctype(sig)
            member = identifier(name)
            class_name = self.Program.Scripts[owner].Name
            self.Program.Seen.add(("class", class_name))
            if is_static:
                expr = f"{class_name}::{member}"
            else:
                expr = f"{'this' if target.Expr == 'this' else self.raw(target)}->{member}"
        else:
            raise TranspileError(f"fields of {owner} are not accessible")

        if op in ("ldfld", "ldsfld"):
            self._push(expr, type_, weak=type_.Kind == "object")
        elif op in ("ldflda", "ldsflda"):
            self._push(expr, byref(type_))
        else:
            self._statement(f"{expr} = {self.coerce(value, type_)};")

    def _arguments(self, args: list, sig) -> str:
        parts = []
        for arg, param in zip(args, sig.Params):
            type_ = self.Program.ctype(param)
            if type_ and type_.Kind == "byref":
                if arg.Type.Kind != "byref":
                    raise TranspileError("expected an address")
                parts.append(arg.Expr)
            else:
                parts.append(self.coerce(arg, type_))
        return ", ".join(parts)

    def _call(self, token: int, is_virtual: bool):
        declaring, name, sig, generic = self.Program.method_ref(self.Assembly, token)
        self._check_owner(declaring)
        args = [self._pop() for _ in sig.Params][::-1]
        target = self._pop() if sig.HasThis else None
        constrained, self.Constrained = self.Constrained, None
        if declaring == "System.Type" and name == "GetTypeFromHandle" and args and args[0].Type.Kind == "typetoken":
            self._push(args[0].Expr, CType("type", args[0].Expr), const=True)
            return
        if declaring == "Artifact.Class" or (constrained == "Artifact.Class" and name == "ToString"):
            self._class_call(name, target, args, generic)
            return
        returns = self.Program.ctype(sig.Return)
        on_this = target is not None and target.Expr == "this"

        if declaring == HANDLER:
            self._interpolation(name, target, args, generic)
        elif declaring == "System.String":
            self._string_call(name, target, args)
        elif name == "ToString" and not sig.Params and (declaring in SYSTEM_VALUE_TYPES or (declaring == "System.Object" and constrained)):
            if constrained and constrained not in SYSTEM_VALUE_TYPES and constrained not in VALUE_STRUCTS:
                raise TranspileError(f"ToString on {constrained} is not supported")
            self._push(f"IL2CPP::ToString({target.Expr})", STRING)
        elif declaring in ("System.MathF", "System.Math") and name in MATH:
            self._math_call(name, args, sig, returns)
        elif declaring in VALUE_STRUCTS:
            self._struct_call(declaring, name, target, args, sig)
        elif name == ".ctor" and on_this and self.IsCtor and (declaring == "Artifact.Object" or self.Program.class_name(declaring)):
            return
        elif declaring == "Artifact.Object":
            self._object_call(name, target)
        elif name in LIFECYCLE and on_this and not is_virtual and declaring not in self.Program.Scripts and self.Program.class_name(declaring):
            return
        elif declaring in self.Program.Scripts:
            class_name = self.Program.Scripts[declaring].Name
            self.Program.Seen.add(("class", class_name))
            method = identifier(name)
            arguments = self._arguments(args, sig)
            if not sig.HasThis:
                self._call_result(f"{class_name}::{method}({arguments})", returns)
            elif on_this and not is_virtual and name in LIFECYCLE:
                self._call_result(f"{class_name}::Script_{name}({arguments})", returns)
            elif on_this and not is_virtual:
                self._call_result(f"{class_name}::{method}({arguments})", returns)
            else:
                self._call_result(f"{self.raw(target)}->{method}({arguments})", returns)
        elif self.Program.class_name(declaring):
            class_name = self.Program.class_name(declaring)
            self.Program.Seen.add(("class", class_name))
            arguments = self._arguments(args, sig)
            if not sig.HasThis:
                self._call_result(f"{class_name}::{name}({arguments})", returns)
            else:
                self._call_result(f"{'this' if on_this else self.raw(target)}->{name}({arguments})", returns)
        else:
            raise TranspileError(f"{declaring}.{name} is not supported")

    def _interpolation(self, name: str, target: Value, args: list, generic: list):
        handler = target.Expr
        if name == ".ctor":
            self._statement(f"{handler} = String();")
        elif name == "AppendLiteral" or (name == "AppendFormatted" and not generic and args[0].Type.Kind == "string"):
            self._statement(f"{handler} += {args[0].Expr};")
        elif name == "AppendFormatted" and len(args) == 1:
            value = args[0]
            if value.Type.Kind in ("object", "null"):
                raise TranspileError("formatting objects is not supported")
            self._statement(f"{handler} += IL2CPP::ToString({value.Expr});")
        elif name == "ToStringAndClear":
            self._call_result(handler, STRING)
        else:
            raise TranspileError(f"string interpolation {name} is not supported")

    def _string_call(self, name: str, target: Value, args: list):
        if name == "Concat":
            parts = args[0].Parts if len(args) == 1 and args[0].Parts is not None else args
            if any(part is None or part.Type.Kind not in ("string", "null") for part in parts):
                raise TranspileError("only strings can be concatenated")
            self._push("(" + " + ".join(self.coerce(part, STRING) for part in parts) + ")", STRING)
        elif name in ("op_Equality", "op_Inequality"):
            self._push(f"({args[0].Expr} {'==' if name == 'op_Equality' else '!='} {args[1].Expr})", BOOL)
        elif name == "get_Length":
            self._push(f"static_cast<int32_t>({target.Expr}.size())", INT32)
        elif name == "IsNullOrEmpty":
            self._push(f"{self.coerce(args[0], STRING)}.empty()", BOOL)
        elif name == "Contains" and len(args) == 1 and args[0].Type.Kind == "string":
            self._push(f"({target.Expr}.find({args[0].Expr}) != String::npos)", BOOL)
        elif name == "ToString":
            self._push(target.Expr, STRING)
        else:
            raise TranspileError(f"System.String.{name} is not supported")

    def _math_call(self, name: str, args: list, sig, returns: CType):
        if name in ("Round",) and len(args) != 1:
            raise TranspileError("Round with digits or a rounding mode is not supported")
        arguments = ", ".join(self.coerce(arg, self.Program.ctype(p)) for arg, p in zip(args, sig.Params))
        template = f"<{returns.Cpp}>" if name in ("Min", "Max", "Clamp") else ""
        self._push(f"{MATH[name]}{template}({arguments})", returns)

    def _struct_call(self, declaring: str, name: str, target: Value, args: list, sig):
        struct_name = VALUE_STRUCTS[declaring]
        type_ = CType("struct", struct_name)
        if name == ".ctor":
            parts = ", ".join(self.coerce(arg, self.Program.ctype(p)) for arg, p in zip(args, sig.Params))
            self._statement(f"{target.Expr} = {struct_name}({parts});")
        elif name in STRUCT_OPERATORS:
            self._push(f"({args[0].Expr} {STRUCT_OPERATORS[name]} {args[1].Expr})", type_)
        elif name == "op_UnaryNegation":
            self._push(f"(-{args[0].Expr})", type_)
        elif name == "Length":
            self._push(f"glm::length({target.Expr})", FLOAT)
        else:
            raise TranspileError(f"{declaring}.{name} is not supported")

    def _class_of(self, value: Value) -> str:
        if value.Type.Kind == "type":
            return f"{value.Type.Cpp}::StaticClass()"
        if value.Type.Kind == "null":
            return "Class::None"
        raise TranspileError("only typeof(...) converts to Artifact.Class")

    def _class_call(self, name: str, target: Value, args: list, generic: list):
        if name == ".ctor":
            self._statement(f"{target.Expr} = Class({self.coerce(args[0], STRING)});")
        elif name == "Of" and generic:
            type_ = self.Program.ctype(generic[0])
            if type_ is None or type_.Kind != "object":
                raise TranspileError("Class.Of<T>() needs an Artifact.Object type")
            self._push(f"{type_.Cpp}::StaticClass()", CLASS)
        elif name in ("Of", "op_Implicit"):
            self._push(self._class_of(args[0]), CLASS)
        elif name == "get_Name" or name == "ToString":
            self._push(f"{target.Expr}.Name", STRING)
        elif name == "get_IsValid":
            self._push(f"({target.Expr} != Class::None)", BOOL)
        elif name == "get_Parent":
            self._push(f"{target.Expr}.GetParentClass()", CLASS)
        elif name == "IsSubclassOf":
            self._push(f"IL2CPP::IsSubclassOf({target.Expr}, {args[0].Expr})", BOOL)
        elif name in ("op_Equality", "op_Inequality"):
            self._push(f"({args[0].Expr} {'==' if name == 'op_Equality' else '!='} {args[1].Expr})", BOOL)
        elif name == "Equals" and args and args[0].Type.Kind == "classref":
            self._push(f"({target.Expr} == {args[0].Expr})", BOOL)
        else:
            raise TranspileError(f"Artifact.Class.{name} is not available in packaged builds")

    def _object_call(self, name: str, target: Value):
        if name == "StaticClass":
            self._push("Object::StaticClass()", CLASS)
        elif name == "GetClass":
            self._push(f"IL2CPP::ClassOf({self.raw(target)})", CLASS)
        elif name == "get_IsValid":
            self._push(f"({self.raw(target)} != nullptr)", BOOL)
        elif name in ("Retain", "Release"):
            self._call_result(f"Object::{name}({self.raw(target)})", None)
        else:
            raise TranspileError(f"Artifact.Object.{name} is not available in packaged builds")
