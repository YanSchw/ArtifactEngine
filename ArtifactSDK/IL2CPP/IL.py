import struct

ONE_BYTE = {
    0x00: ("nop", ""), 0x01: ("break", ""),
    0x02: ("ldarg.0", ""), 0x03: ("ldarg.1", ""), 0x04: ("ldarg.2", ""), 0x05: ("ldarg.3", ""),
    0x06: ("ldloc.0", ""), 0x07: ("ldloc.1", ""), 0x08: ("ldloc.2", ""), 0x09: ("ldloc.3", ""),
    0x0A: ("stloc.0", ""), 0x0B: ("stloc.1", ""), 0x0C: ("stloc.2", ""), 0x0D: ("stloc.3", ""),
    0x0E: ("ldarg.s", "u1"), 0x0F: ("ldarga.s", "u1"), 0x10: ("starg.s", "u1"),
    0x11: ("ldloc.s", "u1"), 0x12: ("ldloca.s", "u1"), 0x13: ("stloc.s", "u1"),
    0x14: ("ldnull", ""), 0x15: ("ldc.i4.m1", ""),
    0x16: ("ldc.i4.0", ""), 0x17: ("ldc.i4.1", ""), 0x18: ("ldc.i4.2", ""), 0x19: ("ldc.i4.3", ""),
    0x1A: ("ldc.i4.4", ""), 0x1B: ("ldc.i4.5", ""), 0x1C: ("ldc.i4.6", ""), 0x1D: ("ldc.i4.7", ""),
    0x1E: ("ldc.i4.8", ""), 0x1F: ("ldc.i4.s", "i1"), 0x20: ("ldc.i4", "i4"), 0x21: ("ldc.i8", "i8"),
    0x22: ("ldc.r4", "r4"), 0x23: ("ldc.r8", "r8"), 0x25: ("dup", ""), 0x26: ("pop", ""),
    0x27: ("jmp", "tok"), 0x28: ("call", "tok"), 0x29: ("calli", "tok"), 0x2A: ("ret", ""),
    0x2B: ("br.s", "br1"), 0x2C: ("brfalse.s", "br1"), 0x2D: ("brtrue.s", "br1"), 0x2E: ("beq.s", "br1"),
    0x2F: ("bge.s", "br1"), 0x30: ("bgt.s", "br1"), 0x31: ("ble.s", "br1"), 0x32: ("blt.s", "br1"),
    0x33: ("bne.un.s", "br1"), 0x34: ("bge.un.s", "br1"), 0x35: ("bgt.un.s", "br1"), 0x36: ("ble.un.s", "br1"),
    0x37: ("blt.un.s", "br1"), 0x38: ("br", "br4"), 0x39: ("brfalse", "br4"), 0x3A: ("brtrue", "br4"),
    0x3B: ("beq", "br4"), 0x3C: ("bge", "br4"), 0x3D: ("bgt", "br4"), 0x3E: ("ble", "br4"), 0x3F: ("blt", "br4"),
    0x40: ("bne.un", "br4"), 0x41: ("bge.un", "br4"), 0x42: ("bgt.un", "br4"), 0x43: ("ble.un", "br4"),
    0x44: ("blt.un", "br4"), 0x45: ("switch", "switch"),
    0x46: ("ldind.i1", ""), 0x47: ("ldind.u1", ""), 0x48: ("ldind.i2", ""), 0x49: ("ldind.u2", ""),
    0x4A: ("ldind.i4", ""), 0x4B: ("ldind.u4", ""), 0x4C: ("ldind.i8", ""), 0x4D: ("ldind.i", ""),
    0x4E: ("ldind.r4", ""), 0x4F: ("ldind.r8", ""), 0x50: ("ldind.ref", ""), 0x51: ("stind.ref", ""),
    0x52: ("stind.i1", ""), 0x53: ("stind.i2", ""), 0x54: ("stind.i4", ""), 0x55: ("stind.i8", ""),
    0x56: ("stind.r4", ""), 0x57: ("stind.r8", ""),
    0x58: ("add", ""), 0x59: ("sub", ""), 0x5A: ("mul", ""), 0x5B: ("div", ""), 0x5C: ("div.un", ""),
    0x5D: ("rem", ""), 0x5E: ("rem.un", ""), 0x5F: ("and", ""), 0x60: ("or", ""), 0x61: ("xor", ""),
    0x62: ("shl", ""), 0x63: ("shr", ""), 0x64: ("shr.un", ""), 0x65: ("neg", ""), 0x66: ("not", ""),
    0x67: ("conv.i1", ""), 0x68: ("conv.i2", ""), 0x69: ("conv.i4", ""), 0x6A: ("conv.i8", ""),
    0x6B: ("conv.r4", ""), 0x6C: ("conv.r8", ""), 0x6D: ("conv.u4", ""), 0x6E: ("conv.u8", ""),
    0x6F: ("callvirt", "tok"), 0x70: ("cpobj", "tok"), 0x71: ("ldobj", "tok"), 0x72: ("ldstr", "tok"),
    0x73: ("newobj", "tok"), 0x74: ("castclass", "tok"), 0x75: ("isinst", "tok"), 0x76: ("conv.r.un", ""),
    0x79: ("unbox", "tok"), 0x7A: ("throw", ""), 0x7B: ("ldfld", "tok"), 0x7C: ("ldflda", "tok"),
    0x7D: ("stfld", "tok"), 0x7E: ("ldsfld", "tok"), 0x7F: ("ldsflda", "tok"), 0x80: ("stsfld", "tok"),
    0x81: ("stobj", "tok"), 0x8C: ("box", "tok"), 0x8D: ("newarr", "tok"), 0x8E: ("ldlen", ""),
    0x8F: ("ldelema", "tok"), 0xA3: ("ldelem", "tok"), 0xA4: ("stelem", "tok"), 0xA5: ("unbox.any", "tok"),
    0xD0: ("ldtoken", "tok"), 0xD1: ("conv.u2", ""), 0xD2: ("conv.u1", ""), 0xD3: ("conv.i", ""),
    0xDC: ("endfinally", ""), 0xDD: ("leave", "br4"), 0xDE: ("leave.s", "br1"), 0xE0: ("conv.u", ""),
}
ONE_BYTE.update({code: (f"ldelem.{suffix}", "") for code, suffix in zip(range(0x90, 0x9B), ["i1", "u1", "i2", "u2", "i4", "u4", "i8", "i", "r4", "r8", "ref"])})
ONE_BYTE.update({code: (f"stelem.{suffix}", "") for code, suffix in zip(range(0x9B, 0xA3), ["i", "i1", "i2", "i4", "i8", "r4", "r8", "ref"])})
ONE_BYTE.update({code: (f"conv.ovf.{suffix}.un", "") for code, suffix in zip(range(0x82, 0x8C), ["i1", "i2", "i4", "i8", "u1", "u2", "u4", "u8", "i", "u"])})
ONE_BYTE.update({code: (f"conv.ovf.{suffix}", "") for code, suffix in zip(range(0xB3, 0xBB), ["i1", "u1", "i2", "u2", "i4", "u4", "i8", "u8"])})
ONE_BYTE.update({code: (name, "") for code, name in zip(range(0xD6, 0xDC), ["add.ovf", "add.ovf.un", "mul.ovf", "mul.ovf.un", "sub.ovf", "sub.ovf.un"])})

TWO_BYTE = {
    0x01: ("ceq", ""), 0x02: ("cgt", ""), 0x03: ("cgt.un", ""), 0x04: ("clt", ""), 0x05: ("clt.un", ""),
    0x06: ("ldftn", "tok"), 0x07: ("ldvirtftn", "tok"), 0x09: ("ldarg", "u2"), 0x0A: ("ldarga", "u2"),
    0x0B: ("starg", "u2"), 0x0C: ("ldloc", "u2"), 0x0D: ("ldloca", "u2"), 0x0E: ("stloc", "u2"),
    0x0F: ("localloc", ""), 0x12: ("unaligned.", "u1"), 0x13: ("volatile.", ""), 0x14: ("tail.", ""),
    0x15: ("initobj", "tok"), 0x16: ("constrained.", "tok"), 0x1A: ("rethrow", ""), 0x1C: ("sizeof", "tok"),
    0x1E: ("readonly.", ""),
}

OPERAND_FORMATS = {"u1": ("<B", 1), "i1": ("<b", 1), "u2": ("<H", 2), "i4": ("<i", 4), "i8": ("<q", 8),
                   "r4": ("<f", 4), "r8": ("<d", 8), "tok": ("<I", 4), "br1": ("<b", 1), "br4": ("<i", 4)}


class Instruction:
    def __init__(self, offset: int, name: str, operand):
        self.Offset = offset
        self.Name = name
        self.Operand = operand

    def __repr__(self):
        return f"IL_{self.Offset:04X}: {self.Name} {self.Operand if self.Operand is not None else ''}"


class ILError(Exception):
    pass


def decode(code: bytes) -> list[Instruction]:
    instructions = []
    position = 0
    while position < len(code):
        offset = position
        opcode = code[position]
        position += 1
        if opcode == 0xFE:
            entry = TWO_BYTE.get(code[position])
            position += 1
        else:
            entry = ONE_BYTE.get(opcode)
        if entry is None:
            raise ILError(f"unknown opcode at IL_{offset:04X}")
        name, kind = entry
        operand = None
        if kind == "switch":
            count = struct.unpack_from("<I", code, position)[0]
            position += 4
            targets = struct.unpack_from(f"<{count}i", code, position)
            position += 4 * count
            operand = [position + target for target in targets]
        elif kind:
            fmt, size = OPERAND_FORMATS[kind]
            operand = struct.unpack_from(fmt, code, position)[0]
            position += size
            if kind in ("br1", "br4"):
                operand += position
        instructions.append(Instruction(offset, name, operand))
    return instructions
