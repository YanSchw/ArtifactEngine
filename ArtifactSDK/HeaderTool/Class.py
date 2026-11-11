from HeaderTool.Properties import generate_properties_registration_code
from HeaderTool.Functions import generate_functions_registration_code

CLASS_CODE = """
using Super = {PARENT_CLASS_NAME};
bool IsObjectOfClass(const Class &type) const override {{
    return type.Name == "{CLASS_NAME}" || Super::IsObjectOfClass(type);
}}
virtual Class GetClass() const override {{
    return Class("{CLASS_NAME}");
}}
static Class StaticClass() {{
    return Class("{CLASS_NAME}");
}}
struct InternalAlloc {{
private:
    InternalAlloc() = default;
    inline static Class::RegisterParentChildClassRelationship _ClassParentChildRelationship_{CLASS_NAME} = {{ "{CLASS_NAME}", "{PARENT_CLASS_NAME}", "{MODULE_NAME}" }};
    inline static Object::RegisterArtifactClass<{CLASS_NAME}> _ClassAllocator_{CLASS_NAME};
    {PROPERTIES_REGISTRATION_CODE}
    {FUNCTIONS_REGISTRATION_CODE}
}}/* No ; is appended after this struct, since the macro itself is supposed to have a ; */
"""

class Class:
    ALL_CLASSES = []

    def __init__(self, name: str, line: int, body: str, parent_class_name: str, module_name: str):
        Class.ALL_CLASSES.append(self)
        self.Name = name
        self.Line = line
        self.Body = body
        self.ParentClassName = parent_class_name
        self.ModuleName = module_name


    def is_a(self, other_class_name: str) -> bool:
        if other_class_name == "Object":
            return True
        if self.Name == other_class_name:
            return True
        
        for parent in Class.ALL_CLASSES:
            if parent.Name == self.ParentClassName:
                return parent.is_a(other_class_name)

        return False

    def generate_class_code(self, gen_file):
        properties_code = "" if self.is_a("Property") else generate_properties_registration_code(
            self.Name,
            self.Body
        )

        functions_code = "" if self.is_a("Property") else generate_functions_registration_code(self.Name, self.Body)

        class_code = CLASS_CODE.format(
            CLASS_NAME=self.Name,
            PARENT_CLASS_NAME=self.ParentClassName,
            MODULE_NAME=self.ModuleName,
            PROPERTIES_REGISTRATION_CODE=properties_code,
            FUNCTIONS_REGISTRATION_CODE=functions_code,
        )

        escaped_class_code = class_code.replace('\n', '\\\n')

        gen_file.write(
            f"#undef _GENERATED_BODY_{self.Line}\n"
            f"#define _GENERATED_BODY_{self.Line} \\\n"
            f"{escaped_class_code}\n\n"
        )
        