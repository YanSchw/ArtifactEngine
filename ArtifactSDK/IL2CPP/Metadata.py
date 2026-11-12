import struct

CODED_INDICES = {
    "TypeDefOrRef": (2, ["TypeDef", "TypeRef", "TypeSpec"]),
    "HasConstant": (2, ["Field", "Param", "Property"]),
    "HasCustomAttribute": (5, ["MethodDef", "Field", "TypeRef", "TypeDef", "Param", "InterfaceImpl", "MemberRef", "Module",
                               "DeclSecurity", "Property", "Event", "StandAloneSig", "ModuleRef", "TypeSpec", "Assembly",
                               "AssemblyRef", "File", "ExportedType", "ManifestResource", "GenericParam",
                               "GenericParamConstraint", "MethodSpec"]),
    "HasFieldMarshal": (1, ["Field", "Param"]),
    "HasDeclSecurity": (2, ["TypeDef", "MethodDef", "Assembly"]),
    "MemberRefParent": (3, ["TypeDef", "TypeRef", "ModuleRef", "MethodDef", "TypeSpec"]),
    "HasSemantics": (1, ["Event", "Property"]),
    "MethodDefOrRef": (1, ["MethodDef", "MemberRef"]),
    "MemberForwarded": (1, ["Field", "MethodDef"]),
    "Implementation": (2, ["File", "AssemblyRef", "ExportedType"]),
    "CustomAttributeType": (3, [None, None, "MethodDef", "MemberRef", None]),
    "ResolutionScope": (2, ["Module", "ModuleRef", "AssemblyRef", "TypeRef"]),
    "TypeOrMethodDef": (1, ["TypeDef", "MethodDef"]),
}

TABLES = [
    ("Module", "u2 Generation, str Name, guid Mvid, guid EncId, guid EncBaseId"),
    ("TypeRef", "c:ResolutionScope Scope, str Name, str Namespace"),
    ("TypeDef", "u4 Flags, str Name, str Namespace, c:TypeDefOrRef Extends, t:Field FieldList, t:MethodDef MethodList"),
    ("FieldPtr", "t:Field Field"),
    ("Field", "u2 Flags, str Name, blob Signature"),
    ("MethodPtr", "t:MethodDef Method"),
    ("MethodDef", "u4 RVA, u2 ImplFlags, u2 Flags, str Name, blob Signature, t:Param ParamList"),
    ("ParamPtr", "t:Param Param"),
    ("Param", "u2 Flags, u2 Sequence, str Name"),
    ("InterfaceImpl", "t:TypeDef Class, c:TypeDefOrRef Interface"),
    ("MemberRef", "c:MemberRefParent Class, str Name, blob Signature"),
    ("Constant", "u1 Type, u1 Padding, c:HasConstant Parent, blob Value"),
    ("CustomAttribute", "c:HasCustomAttribute Parent, c:CustomAttributeType Type, blob Value"),
    ("FieldMarshal", "c:HasFieldMarshal Parent, blob NativeType"),
    ("DeclSecurity", "u2 Action, c:HasDeclSecurity Parent, blob PermissionSet"),
    ("ClassLayout", "u2 PackingSize, u4 ClassSize, t:TypeDef Parent"),
    ("FieldLayout", "u4 Offset, t:Field Field"),
    ("StandAloneSig", "blob Signature"),
    ("EventMap", "t:TypeDef Parent, t:Event EventList"),
    ("EventPtr", "t:Event Event"),
    ("Event", "u2 Flags, str Name, c:TypeDefOrRef EventType"),
    ("PropertyMap", "t:TypeDef Parent, t:Property PropertyList"),
    ("PropertyPtr", "t:Property Property"),
    ("Property", "u2 Flags, str Name, blob Type"),
    ("MethodSemantics", "u2 Semantics, t:MethodDef Method, c:HasSemantics Association"),
    ("MethodImpl", "t:TypeDef Class, c:MethodDefOrRef Body, c:MethodDefOrRef Declaration"),
    ("ModuleRef", "str Name"),
    ("TypeSpec", "blob Signature"),
    ("ImplMap", "u2 Flags, c:MemberForwarded Member, str ImportName, t:ModuleRef ImportScope"),
    ("FieldRVA", "u4 RVA, t:Field Field"),
    ("EncLog", "u4 Token, u4 FuncCode"),
    ("EncMap", "u4 Token"),
    ("Assembly", "u4 HashAlgId, u2 Major, u2 Minor, u2 Build, u2 Revision, u4 Flags, blob PublicKey, str Name, str Culture"),
    ("AssemblyProcessor", "u4 Processor"),
    ("AssemblyOS", "u4 Platform, u4 Major, u4 Minor"),
    ("AssemblyRef", "u2 Major, u2 Minor, u2 Build, u2 Revision, u4 Flags, blob PublicKey, str Name, str Culture, blob Hash"),
    ("AssemblyRefProcessor", "u4 Processor, t:AssemblyRef AssemblyRef"),
    ("AssemblyRefOS", "u4 Platform, u4 Major, u4 Minor, t:AssemblyRef AssemblyRef"),
    ("File", "u4 Flags, str Name, blob Hash"),
    ("ExportedType", "u4 Flags, u4 TypeDefId, str Name, str Namespace, c:Implementation Implementation"),
    ("ManifestResource", "u4 Offset, u4 Flags, str Name, c:Implementation Implementation"),
    ("NestedClass", "t:TypeDef Nested, t:TypeDef Enclosing"),
    ("GenericParam", "u2 Number, u2 Flags, c:TypeOrMethodDef Owner, str Name"),
    ("MethodSpec", "c:MethodDefOrRef Method, blob Instantiation"),
    ("GenericParamConstraint", "t:GenericParam Owner, c:TypeDefOrRef Constraint"),
]
TABLE_IDS = {name: index for index, (name, _) in enumerate(TABLES)}
TOKEN_TABLES = {0x01: "TypeRef", 0x02: "TypeDef", 0x04: "Field", 0x06: "MethodDef", 0x0A: "MemberRef",
                0x11: "StandAloneSig", 0x1B: "TypeSpec", 0x2B: "MethodSpec"}

ELEMENT_NAMES = {0x01: "void", 0x02: "bool", 0x03: "char", 0x04: "i1", 0x05: "u1", 0x06: "i2", 0x07: "u2", 0x08: "i4",
                 0x09: "u4", 0x0A: "i8", 0x0B: "u8", 0x0C: "r4", 0x0D: "r8", 0x0E: "string", 0x18: "intptr",
                 0x19: "uintptr", 0x1C: "object", 0x16: "typedref"}


class MetadataError(Exception):
    pass


class Row:
    def __init__(self, table: str, index: int, values: dict):
        self.Table = table
        self.Index = index
        self.__dict__.update(values)


class TypeSig:
    def __init__(self, kind: str, name: str = "", element=None, args=None, number: int = 0):
        self.Kind = kind
        self.Name = name
        self.Element = element
        self.Args = args or []
        self.Number = number

    def __repr__(self):
        if self.Kind in ("class", "valuetype"):
            return self.Name
        if self.Kind == "generic":
            return f"{self.Name}<{', '.join(map(repr, self.Args))}>"
        if self.Kind in ("byref", "szarray", "ptr"):
            return f"{self.Kind}<{self.Element}>"
        return self.Kind


class MethodSig:
    def __init__(self, has_this: bool, generic_count: int, ret: TypeSig, params: list):
        self.HasThis = has_this
        self.GenericCount = generic_count
        self.Return = ret
        self.Params = params


class MethodBody:
    def __init__(self, code: bytes, locals_token: int, has_exception_handlers: bool):
        self.Code = code
        self.LocalsToken = locals_token
        self.HasExceptionHandlers = has_exception_handlers


class Assembly:
    def __init__(self, path: str):
        with open(path, "rb") as file:
            self.Data = file.read()
        self._read_pe()
        self._read_metadata_root()
        self._read_tables()

    def _u16(self, offset):
        return struct.unpack_from("<H", self.Data, offset)[0]

    def _u32(self, offset):
        return struct.unpack_from("<I", self.Data, offset)[0]

    def _read_pe(self):
        if self.Data[:2] != b"MZ":
            raise MetadataError("not a PE file")
        pe = self._u32(0x3C)
        if self.Data[pe:pe + 4] != b"PE\0\0":
            raise MetadataError("missing PE signature")
        section_count = self._u16(pe + 6)
        optional_size = self._u16(pe + 20)
        optional = pe + 24
        directories = optional + (96 if self._u16(optional) == 0x10B else 112)
        cli_rva = self._u32(directories + 14 * 8)
        self.Sections = []
        section = optional + optional_size
        for i in range(section_count):
            base = section + i * 40
            self.Sections.append((self._u32(base + 12), self._u32(base + 8), self._u32(base + 20)))
        cli = self.rva_to_offset(cli_rva)
        self.MetadataOffset = self.rva_to_offset(self._u32(cli + 8))

    def rva_to_offset(self, rva: int) -> int:
        for address, size, raw in self.Sections:
            if address <= rva < address + size:
                return rva - address + raw
        raise MetadataError(f"RVA {rva:#x} outside every section")

    def _read_metadata_root(self):
        root = self.MetadataOffset
        if self._u32(root) != 0x424A5342:
            raise MetadataError("missing metadata signature")
        version_length = self._u32(root + 12)
        cursor = root + 16 + version_length
        stream_count = self._u16(cursor + 2)
        cursor += 4
        self.Streams = {}
        for _ in range(stream_count):
            offset, size = self._u32(cursor), self._u32(cursor + 4)
            end = self.Data.index(b"\0", cursor + 8)
            name = self.Data[cursor + 8:end].decode()
            cursor = (end + 4) & ~3
            self.Streams[name] = (root + offset, size)

    def _read_tables(self):
        start, _ = self.Streams.get("#~") or self.Streams["#-"]
        heap_sizes = self.Data[start + 6]
        self.StringSize = 4 if heap_sizes & 1 else 2
        self.GuidSize = 4 if heap_sizes & 2 else 2
        self.BlobSize = 4 if heap_sizes & 4 else 2
        valid = struct.unpack_from("<Q", self.Data, start + 8)[0]
        cursor = start + 24
        self.RowCounts = [0] * 64
        for bit in range(64):
            if valid & (1 << bit):
                self.RowCounts[bit] = self._u32(cursor)
                cursor += 4
        if heap_sizes & 0x40:
            cursor += 4

        self.Tables = {}
        for index, (name, schema) in enumerate(TABLES):
            columns = [self._column(column.strip()) for column in schema.split(",")]
            rows = []
            for row_index in range(1, self.RowCounts[index] + 1):
                values = {}
                for column_name, size, decode in columns:
                    raw = int.from_bytes(self.Data[cursor:cursor + size], "little")
                    values[column_name] = decode(raw)
                    cursor += size
                rows.append(Row(name, row_index, values))
            self.Tables[name] = rows
        if any(self.RowCounts[len(TABLES):]):
            raise MetadataError("unexpected metadata tables")

    def _column(self, column: str):
        kind, name = column.split(" ")
        if kind in ("u1", "u2", "u4"):
            return name, int(kind[1]), lambda raw: raw
        if kind == "str":
            return name, self.StringSize, self.string
        if kind == "guid":
            return name, self.GuidSize, lambda raw: raw
        if kind == "blob":
            return name, self.BlobSize, lambda raw: raw
        if kind.startswith("t:"):
            table = kind[2:]
            return name, 2 if self.RowCounts[TABLE_IDS[table]] < 0x10000 else 4, lambda raw: raw
        tag_bits, tables = CODED_INDICES[kind[2:]]
        largest = max(self.RowCounts[TABLE_IDS[table]] for table in tables if table)
        size = 2 if largest < (1 << (16 - tag_bits)) else 4
        mask = (1 << tag_bits) - 1
        return name, size, lambda raw: (tables[raw & mask] if (raw & mask) < len(tables) else None, raw >> tag_bits)

    def string(self, offset: int) -> str:
        start = self.Streams["#Strings"][0] + offset
        return self.Data[start:self.Data.index(b"\0", start)].decode("utf-8")

    def blob(self, offset: int) -> bytes:
        start = self.Streams["#Blob"][0] + offset
        length, size = decode_compressed(self.Data, start)
        return self.Data[start + size:start + size + length]

    def user_string(self, offset: int) -> str:
        start = self.Streams["#US"][0] + offset
        length, size = decode_compressed(self.Data, start)
        return self.Data[start + size:start + size + length - 1].decode("utf-16-le")

    def row(self, table: str, index: int) -> Row:
        return self.Tables[table][index - 1]

    def token(self, token: int) -> Row:
        table = TOKEN_TABLES.get(token >> 24)
        if table is None:
            raise MetadataError(f"unsupported token {token:#010x}")
        return self.row(table, token & 0xFFFFFF)

    def range_of(self, owner: Row, list_column: str, table: str) -> list:
        rows = self.Tables[owner.Table]
        start = getattr(owner, list_column)
        end = getattr(rows[owner.Index], list_column) if owner.Index < len(rows) else len(self.Tables[table]) + 1
        return [self.row(table, i) for i in range(start, end)]

    def fields_of(self, type_def: Row) -> list:
        return self.range_of(type_def, "FieldList", "Field")

    def methods_of(self, type_def: Row) -> list:
        return self.range_of(type_def, "MethodList", "MethodDef")

    def params_of(self, method: Row) -> list:
        return self.range_of(method, "ParamList", "Param")

    def type_name(self, coded) -> str:
        table, index = coded
        if table is None or index == 0:
            return ""
        row = self.row(table, index)
        if table == "TypeSpec":
            return repr(self.decode_type(SigReader(self, self.blob(row.Signature))))
        if table == "TypeRef" and row.Scope[0] == "TypeRef":
            return self.type_name(row.Scope) + "/" + row.Name
        return f"{row.Namespace}.{row.Name}" if row.Namespace else row.Name

    def declaring_type(self, method: Row) -> Row:
        for type_def in self.Tables["TypeDef"]:
            if any(m.Index == method.Index for m in self.methods_of(type_def)):
                return type_def
        raise MetadataError(f"method {method.Name} has no declaring type")

    def field_owner(self, field: Row) -> Row:
        for type_def in self.Tables["TypeDef"]:
            if any(f.Index == field.Index for f in self.fields_of(type_def)):
                return type_def
        raise MetadataError(f"field {field.Name} has no declaring type")

    def custom_attributes(self, table: str, index: int) -> list:
        names = []
        for attribute in self.Tables["CustomAttribute"]:
            if attribute.Parent == (table, index):
                ctor_table, ctor_index = attribute.Type
                ctor = self.row(ctor_table, ctor_index)
                owner = self.type_name(ctor.Class) if ctor_table == "MemberRef" else self._type_def_name(self.declaring_type(ctor))
                names.append(owner)
        return names

    def _type_def_name(self, type_def: Row) -> str:
        return f"{type_def.Namespace}.{type_def.Name}" if type_def.Namespace else type_def.Name

    def method_sig(self, blob_offset: int) -> MethodSig:
        reader = SigReader(self, self.blob(blob_offset))
        convention = reader.byte()
        generic_count = reader.compressed() if convention & 0x10 else 0
        count = reader.compressed()
        ret = self.decode_type(reader)
        return MethodSig(bool(convention & 0x20), generic_count, ret, [self.decode_type(reader) for _ in range(count)])

    def field_sig(self, blob_offset: int) -> TypeSig:
        reader = SigReader(self, self.blob(blob_offset))
        if reader.byte() != 0x06:
            raise MetadataError("malformed field signature")
        return self.decode_type(reader)

    def locals_sig(self, token: int) -> list:
        if token == 0:
            return []
        reader = SigReader(self, self.blob(self.token(token).Signature))
        if reader.byte() != 0x07:
            raise MetadataError("malformed locals signature")
        return [self.decode_type(reader) for _ in range(reader.compressed())]

    def method_spec_args(self, blob_offset: int) -> list:
        reader = SigReader(self, self.blob(blob_offset))
        if reader.byte() != 0x0A:
            raise MetadataError("malformed method spec")
        return [self.decode_type(reader) for _ in range(reader.compressed())]

    def decode_type(self, reader) -> TypeSig:
        element = reader.byte()
        while element in (0x1F, 0x20):
            reader.compressed()
            element = reader.byte()
        if element in ELEMENT_NAMES:
            return TypeSig(ELEMENT_NAMES[element])
        if element in (0x10, 0x0F, 0x1D, 0x45):
            inner = self.decode_type(reader)
            return inner if element == 0x45 else TypeSig({0x10: "byref", 0x0F: "ptr", 0x1D: "szarray"}[element], element=inner)
        if element in (0x11, 0x12):
            coded = reader.compressed()
            name = self.type_name((["TypeDef", "TypeRef", "TypeSpec"][coded & 3], coded >> 2))
            return TypeSig("valuetype" if element == 0x11 else "class", name)
        if element == 0x15:
            generic = self.decode_type(reader)
            args = [self.decode_type(reader) for _ in range(reader.compressed())]
            return TypeSig("generic", generic.Name, element=generic, args=args)
        if element in (0x13, 0x1E):
            return TypeSig("var" if element == 0x13 else "mvar", number=reader.compressed())
        raise MetadataError(f"unsupported element type {element:#x}")

    def method_body(self, method: Row) -> MethodBody:
        if method.RVA == 0:
            return None
        offset = self.rva_to_offset(method.RVA)
        header = self.Data[offset]
        if header & 3 == 2:
            return MethodBody(self.Data[offset + 1:offset + 1 + (header >> 2)], 0, False)
        flags = self._u16(offset)
        size = (flags >> 12) * 4
        code_size = self._u32(offset + 4)
        locals_token = self._u32(offset + 8)
        return MethodBody(self.Data[offset + size:offset + size + code_size], locals_token, bool(flags & 0x08))


class SigReader:
    def __init__(self, assembly: Assembly, data: bytes):
        self.Assembly = assembly
        self.Data = data
        self.Position = 0

    def byte(self) -> int:
        value = self.Data[self.Position]
        self.Position += 1
        return value

    def compressed(self) -> int:
        value, size = decode_compressed(self.Data, self.Position)
        self.Position += size
        return value


def decode_compressed(data: bytes, offset: int):
    first = data[offset]
    if first & 0x80 == 0:
        return first, 1
    if first & 0xC0 == 0x80:
        return ((first & 0x3F) << 8) | data[offset + 1], 2
    return ((first & 0x1F) << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8) | data[offset + 3], 4
