
ENUM_CODE = """
{FORWARD_DECLARATION}
struct E{ENUM_NAME} {{
    static Enum StaticEnum() {{
        return Enum("{ENUM_NAME}");
    }}
}}
"""

class Enum:
    def __init__(self, name: str, line: int, body: str, is_enum_class: bool, underlying_type: str):
        self.Name = name
        self.Line = line
        self.Body = body
        self.IsEnumClass = is_enum_class
        self.UnderlyingType = underlying_type

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
    UNDERLYING_TYPE=self.UnderlyingType
).replace('\n', '\\\n')}

''')