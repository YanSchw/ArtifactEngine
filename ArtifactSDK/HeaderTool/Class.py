
CLASS_CODE = """
using Super = {PARENT_CLASS_NAME};
bool IsObjectOfClass(const Class &type) const override {{
    return type.Name == "{CLASS_NAME}" || Super::IsObjectOfClass(type);
}}
static Class StaticClass() {{
    return Class("{CLASS_NAME}");
}}
struct InternalAlloc {{
private:
    InternalAlloc() = default;
    inline static Object::RegisterArtifactClass<{CLASS_NAME}> _ClassAllocator_{CLASS_NAME};
}}/* No ; is appended after this struct, since the macro itself is supposed to have a ; */
"""

class Class:
    def __init__(self, name: str, line: int, body: str, parent_class_name: str):
        self.Name = name
        self.Line = line
        self.Body = body
        self.ParentClassName = parent_class_name

    def generate_class_code(self, gen_file):
        gen_file.write(f'''#define _GENERATED_BODY_{self.Line} \\
{CLASS_CODE.format(
    CLASS_NAME=self.Name,
    PARENT_CLASS_NAME=self.ParentClassName,
).replace('\n', '\\\n')}

''')
        