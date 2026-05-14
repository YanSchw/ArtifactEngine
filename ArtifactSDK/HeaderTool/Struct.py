from HeaderTool.Properties import generate_properties_registration_code

STRUCT_CODE = """
static Struct StaticStruct() {{
    return Struct("{STRUCT_NAME}");
}}
struct InternalAlloc {{
private:
    InternalAlloc() = default;
    {PROPERTIES_REGISTRATION_CODE}
}}/* No ; is appended after this struct, since the macro itself is supposed to have a ; */
"""

class Struct:
    ALL_STRUCTS = []

    def __init__(self, name: str, line: int, body: str):
        Struct.ALL_STRUCTS.append(self)
        self.Name = name
        self.Line = line
        self.Body = body

    def generate_struct_code(self, gen_file):
        gen_file.write(f'''#define _GENERATED_BODY_{self.Line} \\
{STRUCT_CODE.format(
    STRUCT_NAME=self.Name,
    PROPERTIES_REGISTRATION_CODE=generate_properties_registration_code(self.Name, self.Body),
).replace('\n', '\\\n')}

''')