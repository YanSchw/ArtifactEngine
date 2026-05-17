import re

PROPERTY_PATTERN = re.compile(
        r'PROPERTY\s*\(\s*\)\s*'      # PROPERTY()
        r'([\w:<> ,]+?)'              # typename
        r'\s+'
        r'(\w+)'                      # property name
        r'(?:\s*=\s*[^;]+)?'          # optional initializer
        r'\s*;',
        re.MULTILINE
    )

SIMPLE_PROPERTY = """static {PROPERTY_TYPE} _Property_{PROPERTY_NAME} = {PROPERTY_TYPE}("{PROPERTY_NAME}", {OFFSET}{INITIALIZER});"""

ARRAY_GETSIZE = """
/* GetSize */
[](void* ptr) -> size_t {{
    auto& arr = *reinterpret_cast<Array<{PROPERTY_TYPE}>*>(ptr);
    return arr.Size();
}}
"""

def generate_property_type(full_typename: str, prop_name: str, class_or_struct_typename: str, use_offset: bool = True) -> str:
    offset = f"offsetof({class_or_struct_typename}, {prop_name})" if use_offset else "0"

    if full_typename == "uint8_t" or full_typename == "uint8":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="IntProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", true, 8")
    if full_typename == "uint16_t" or full_typename == "uint16":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="IntProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", true, 16")
    if full_typename == "uint32_t" or full_typename == "uint32":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="IntProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", true, 32")
    if full_typename == "uint64_t" or full_typename == "uint64":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="IntProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", true, 64")
    

    if full_typename == "int8_t" or full_typename == "int8":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="IntProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", false, 8")
    if full_typename == "int16_t" or full_typename == "int16":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="IntProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", false, 16")
    if full_typename == "int32_t" or full_typename == "int32":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="IntProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", false, 32")
    if full_typename == "int64_t" or full_typename == "int64":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="IntProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", false, 64")
    

    if full_typename == "float":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="FloatProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", false")
    if full_typename == "double":
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="FloatProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=", true")

    if full_typename.startswith("SharedObjectPtr<") and full_typename.endswith(">"):
        inner_type = full_typename[len("SharedObjectPtr<"):-1].strip()
        return SIMPLE_PROPERTY.format(PROPERTY_TYPE="SharedObjectPtrProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=f', Class("{inner_type}")')

    if full_typename.startswith("Array<") and full_typename.endswith(">"):
        inner_type = full_typename[len("Array<"):-1].strip()
        inner_type_str = generate_property_type(inner_type, prop_name + "_InnerArrayProperty", class_or_struct_typename, use_offset=False)
        return inner_type_str + "\n" + SIMPLE_PROPERTY.format(PROPERTY_TYPE="ArrayProperty", PROPERTY_NAME=prop_name, OFFSET=offset, INITIALIZER=f', &_Property_{prop_name}_InnerArrayProperty,{ARRAY_GETSIZE.format(PROPERTY_TYPE=inner_type)}')

    raise NotImplementedError(f"Property type '{full_typename}' is not supported yet.")


def generate_properties_registration_code(typename: str, body: str) -> str:
    if typename.endswith("Property"):
        return ""

    raw_properties = []
    for m in PROPERTY_PATTERN.finditer(body):
        type_name = m.group(1).strip().replace(' ', '')
        prop_name = m.group(2).strip()
        raw_properties.append((type_name, prop_name))

    create_properties_list = [generate_property_type(type_name, prop_name, typename) for type_name, prop_name in raw_properties]
    create_properties = "\n".join(create_properties_list)
    properties_list = "\n                ".join(
        f"&_Property_{prop_name}," for _, prop_name in raw_properties
    )
    return f"""struct InternalRegisterProperties {{
        InternalRegisterProperties() {{
            {typename}* _NullInstance = nullptr;
{"\n".join("            " + line for line in create_properties.splitlines())}
            Property::RegisterTypeProperties("{typename}", {{
                {properties_list}
            }});
        }}
    }};
    inline static InternalRegisterProperties _InternalRegisterProperties;
"""