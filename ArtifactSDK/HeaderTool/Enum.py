import re


ENUM_CODE = """
{FORWARD_DECLARATION}
struct E{ENUM_NAME} {{
    static Enum StaticEnum() {{
        return Enum("{ENUM_NAME}");
    }}

    static {UNDERLYING_TYPE} ConvertStringToValue(const String& InString) {{
        return static_cast<{UNDERLYING_TYPE}>(StaticEnum().ConvertStringToValue(InString));
    }}
    static {ENUM_NAME} ConvertStringToEnum(const String& InString) {{
        return static_cast<{ENUM_NAME}>(StaticEnum().ConvertStringToValue(InString));
    }}
    static String ConvertValueToString({UNDERLYING_TYPE} InValue) {{
        return StaticEnum().ConvertValueToString(static_cast<int64_t>(InValue));
    }}
    static String ConvertEnumToString({ENUM_NAME} InValue) {{
        return StaticEnum().ConvertValueToString(static_cast<int64_t>(InValue));
    }}

    inline static Enum::RegisterEnumValues _EnumValues_{ENUM_NAME} = Enum::RegisterEnumValues("{ENUM_NAME}", {{
{ENUM_VALUES}
    }});
}}
"""

class Enum:
    def __init__(self, name: str, line: int, body: str, is_enum_class: bool, underlying_type: str):
        self.Name = name
        self.Line = line
        self.Body = body
        self.IsEnumClass = is_enum_class
        self.UnderlyingType = underlying_type

    def parse_enum_values(self):
        entries = [
            e.strip()
            for e in self.Body.split(',')
            if e.strip()
        ]

        values = []
        current = 0

        for entry in entries:
            m = re.match(r'(\w+)(?:\s*=\s*(.+))?$', entry)
            if not m:
                continue

            name = m.group(1)
            expr = m.group(2)

            if expr:
                current = eval(expr, {"__builtins__": None}, {})

            values.append((name, current))
            current += 1

        return values

    def generate_enum_code(self, gen_file):
        forward_declaration = "enum "
        if self.IsEnumClass:
            forward_declaration += "class "
        forward_declaration += self.Name
        if self.UnderlyingType:
            forward_declaration += f" : {self.UnderlyingType}"
        forward_declaration += ";"

        gen_file.write(f'''#define _GENERATED_BODY_{self.Line} \\
{ENUM_CODE.format(
    FORWARD_DECLARATION=forward_declaration,
    ENUM_NAME=self.Name,
    UNDERLYING_TYPE=self.UnderlyingType or 'int',
    ENUM_VALUES='\n'.join([f'        {{"{name}", {value}}},' for name, value in self.parse_enum_values()]),
).replace('\n', '\\\n')}

''')