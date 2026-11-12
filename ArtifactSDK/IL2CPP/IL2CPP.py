import json
import os
from IL2CPP.Metadata import Assembly
from IL2CPP.Translator import Program, Script, MethodTranslator, TranspileError, CType, LIFECYCLE, identifier
from SDK.Util import smart_open

RUNTIME_HEADER = os.path.join(os.path.dirname(__file__), "IL2CPPRuntime.h")
PROPERTY_KINDS = {("bool", "bool"), ("int", "int32_t"), ("float", "float"), ("float", "double"), ("string", "String"),
                  ("struct", "Vec2"), ("struct", "Vec3"), ("struct", "Vec4"), ("struct", "Color")}
LIFECYCLE_SIGNATURES = {"BeginPlay": [], "EndPlay": [], "WorldUpdate": ["r4"]}
TYPE_INTERFACE = 0x20
METHOD_STATIC, METHOD_VIRTUAL, METHOD_NEW_SLOT, METHOD_ABSTRACT = 0x10, 0x40, 0x100, 0x400
FIELD_STATIC, FIELD_LITERAL = 0x10, 0x40


class ClassEmitter:
    def __init__(self, program: Program, script: Script, output_dir: str):
        self.Program = program
        self.Script = script
        self.Assembly = script.Assembly
        self.Type = script.Type
        self.Name = script.Name
        self.OutputDir = output_dir
        self.Parent = program.class_name(script.Parent)
        self.ParentScript = program.Scripts.get(script.Parent)
        native = script.Parent
        while native in program.Scripts:
            native = program.Scripts[native].Parent
        self.NativeBase = program.class_name(native)
        self.IsNode = self._native_is_node(self.NativeBase)
        self.Modules = {"EngineCore"}

    def script_header(self, script: Script) -> str:
        if script.Module == self.Script.Module:
            return f'#include "{script.Name}.Script.h"'
        self.Modules.add(script.Module)
        return f'#include "{self.OutputDir}/{script.Module}/Source/{script.Name}.Script.h"'

    def _native_is_node(self, name: str) -> bool:
        while name:
            if name == "Node":
                return True
            name = self.Program.Classes.get(name, {}).get("Parent")
        return False

    def _error(self, message: str):
        raise TranspileError(f"{self.Name}: {message}")

    def _validate(self):
        if self.Type.Flags & TYPE_INTERFACE:
            self._error("interfaces are not supported")
        if any(row.Nested == self.Type.Index for row in self.Assembly.Tables["NestedClass"]):
            self._error("nested classes are not supported")
        if any(row.Owner == ("TypeDef", self.Type.Index) for row in self.Assembly.Tables["GenericParam"]):
            self._error("generic classes are not supported")
        if any(row.Class == self.Type.Index for row in self.Assembly.Tables["InterfaceImpl"]):
            self._error("implementing interfaces is not supported")

    def _ctype(self, sig, where: str) -> CType:
        try:
            return self.Program.ctype(sig)
        except TranspileError as error:
            raise TranspileError(f"{self.Name}.{where}: {error}")

    def _params(self, method, sig) -> list:
        names = {param.Sequence: param.Name for param in self.Assembly.params_of(method)}
        return [(identifier(names.get(i + 1) or f"p{i}"), self._ctype(param, method.Name)) for i, param in enumerate(sig.Params)]

    def _includes(self, header: bool) -> list[str]:
        includes = set()
        for kind, name in sorted(self.Program.Seen):
            if name == self.Name:
                continue
            script = next((script for script in self.Program.Scripts.values() if script.Name == name), None)
            if kind == "class" and script:
                includes.add(f"class {name};" if header else self.script_header(script))
                continue
            info = (self.Program.Classes if kind == "class" else self.Program.Enums).get(name)
            if info and info.get("Header"):
                includes.add(f'#include "{info["Header"]}"')
                if info.get("Module"):
                    self.Modules.add(info["Module"])
        return sorted(includes, key=lambda line: (line.startswith("class"), line))

    def emit(self):
        self._validate()
        assembly = self.Assembly
        self.Program.Seen = set()
        members = []
        for field in assembly.fields_of(self.Type):
            if field.Flags & FIELD_LITERAL:
                continue
            type_ = self._ctype(assembly.field_sig(field.Signature), field.Name)
            is_static = field.Flags & FIELD_STATIC
            is_property = "Artifact.PropertyAttribute" in assembly.custom_attributes("Field", field.Index)
            if is_property and (is_static or (type_.Kind, type_.Cpp) not in PROPERTY_KINDS):
                self._error(f"[Property] {field.Name} must be an instance float, double, int, bool, string, Vec2, Vec3, Vec4 or Color")
            zero = type_.zero(True)
            declaration = f"{'static inline ' if is_static else ''}{type_.decl(True)} {identifier(field.Name)}{f' = {zero}' if zero else ''};"
            members.append(("    PROPERTY()\n" if is_property else "") + f"    {declaration}")

        methods = []
        constructor = None
        ticks = False
        for method in assembly.methods_of(self.Type):
            sig = assembly.method_sig(method.Signature)
            if method.Name == ".cctor":
                self._error("static constructors and static field initializers are not supported")
            if sig.GenericCount:
                self._error(f"generic method {method.Name} is not supported")
            if method.Flags & METHOD_ABSTRACT:
                self._error(f"abstract method {method.Name} is not supported")
            params = self._params(method, sig)
            if method.Name == ".ctor":
                if sig.Params:
                    self._error("only a parameterless constructor is supported")
                constructor = method
                continue
            is_virtual = method.Flags & METHOD_VIRTUAL
            lifecycle = (self.IsNode and method.Name in LIFECYCLE and is_virtual and not method.Flags & METHOD_NEW_SLOT
                         and [p.Kind for p in sig.Params] == LIFECYCLE_SIGNATURES[method.Name] and sig.Return.Kind == "void")
            ticks = ticks or (lifecycle and method.Name == "WorldUpdate")
            methods.append((method, sig, params, lifecycle))

        declarations = [f"    {self.Name}();"]
        for method, sig, params, lifecycle in methods:
            signature = ", ".join(f"{type_.decl(False)} {name}" for name, type_ in params)
            if lifecycle:
                declarations.append(f"    virtual void {method.Name}({signature}) override;")
                declarations.append(f"    void Script_{method.Name}({signature});")
                continue
            returns = self._ctype(sig.Return, method.Name)
            prefix = "static " if method.Flags & METHOD_STATIC else ("virtual " if method.Flags & METHOD_VIRTUAL else "")
            suffix = " override" if method.Flags & METHOD_VIRTUAL and not method.Flags & METHOD_NEW_SLOT \
                and self.Program.overrides_in_script(self.Script, method.Name) else ""
            declarations.append(f"    {prefix}{returns.decl(False) if returns else 'void'} {identifier(method.Name)}({signature}){suffix};")

        parent_include = self.script_header(self.ParentScript) if self.ParentScript else None
        if not parent_include:
            parent_header = self.Program.Classes.get(self.Parent, {}).get("Header")
            parent_include = f'#include "{parent_header}"' if parent_header else '#include "Object/Object.h"'
            self.Modules.add(self.Program.Classes.get(self.Parent, {}).get("Module") or "EngineCore")
        header_includes = [parent_include] + [line for line in self._includes(True) if line != parent_include]
        includes = [line for line in header_includes if line.startswith("#include")]
        forwards = [line for line in header_includes if line.startswith("class")]

        header = ["#pragma once", *includes, f'#include "{self.Name}.Script.gen.h"', ""]
        if forwards:
            header += forwards + [""]
        header += [f"class {self.Name} : public {self.Parent} {{", "public:", "    ARTIFACT_CLASS();", ""]
        header += declarations
        if members:
            header += [""] + "\n\n".join(members).split("\n")
        header += ["};", ""]

        self.Program.Seen = set()
        bodies = []
        bodies.append(self._definition(f"{self.Name}::{self.Name}()", constructor, [], True,
                                       ["SetUpdateFlag(UpdateFlag::WorldUpdate);"] if ticks else []))
        for method, sig, params, lifecycle in methods:
            signature = ", ".join(f"{type_.decl(False)} {name}" for name, type_ in params)
            arguments = ", ".join(name for name, _ in params)
            if lifecycle:
                native = f"{self.NativeBase}::{method.Name}({arguments});"
                script = f"Script_{method.Name}({arguments});"
                order = [script, native] if method.Name == "EndPlay" else [native, script]
                bodies.append(f"void {self.Name}::{method.Name}({signature}) {{\n" + "".join(f"    {line}\n" for line in order) + "}")
                bodies.append(self._definition(f"void {self.Name}::Script_{method.Name}({signature})", method, params, False))
                continue
            returns = self._ctype(sig.Return, method.Name)
            bodies.append(self._definition(f"{returns.decl(False) if returns else 'void'} {self.Name}::{identifier(method.Name)}({signature})",
                                           method, params, False))

        source = [f'#include "{self.Name}.Script.h"', '#include "IL2CPPRuntime.h"'] + self._includes(False) + [""]
        source.append("\n\n".join(bodies))
        return "\n".join(header), "\n".join(source) + "\n"

    def _definition(self, signature: str, method, params: list, is_ctor: bool, prologue: list = None) -> str:
        lines = list(prologue or [])
        if method is not None and self.Assembly.method_body(method) is not None:
            sig = self.Assembly.method_sig(method.Signature)
            lines += MethodTranslator(self.Program, self.Script, method, sig, params, is_ctor).translate()
        return f"{signature} {{\n" + "".join(f"    {line}\n" for line in lines) + "}"


def transpile(modules: list[tuple[str, str]], reflection_path: str, output_dir: str):
    with open(reflection_path, encoding="utf-8") as file:
        program = Program({name: Assembly(path) for name, path in modules}, json.load(file))

    for script in program.Scripts.values():
        if script.Name in program.Classes:
            raise TranspileError(f"{script.Name}: an engine class with this name already exists")

    with open(RUNTIME_HEADER, encoding="utf-8") as runtime:
        runtime_header = runtime.read()

    for module, _ in modules:
        source_dir = f"{output_dir}/{module}/Source"
        imports = {"EngineCore"}
        for script in (script for script in program.Scripts.values() if script.Module == module):
            emitter = ClassEmitter(program, script, output_dir)
            header, source = emitter.emit()
            imports |= emitter.Modules
            write_file(f"{source_dir}/{script.Name}.Script.h", header)
            write_file(f"{source_dir}/{script.Name}.Script.cpp", source)
        write_file(f"{source_dir}/IL2CPPRuntime.h", runtime_header)
        definition = {"SourceDirectories": ["Source"], "IncludePaths": ["Source"], "ImportModules": sorted(imports - {module})}
        write_file(f"{output_dir}/{module}/Module.json", json.dumps(definition, indent=4) + "\n")


def write_file(path: str, content: str):
    with smart_open(path) as file:
        file.write(content)
