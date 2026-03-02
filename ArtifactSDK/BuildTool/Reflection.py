import os
import re

def get_module_name_from_path(module_path: str) -> str:
    module_path = module_path.replace("\\", "/")
    pattern = re.compile(r'.*/Modules/([^/]+)')
    match = pattern.match(module_path)
    if match:
        return match.group(1)
    else:
        raise ValueError(f"Could not extract module name from path: {module_path}")
    
def collect_classes_from_header(h_file_path: str):
    with open(h_file_path, 'r', encoding='utf-8') as h_file:
        code = h_file.read()
        code_lines = code.splitlines()
        pattern = re.compile(r'class\s+(\w+)\s*:\s*public\s+(\w+)\s*\{', re.MULTILINE)
        results = []
        for match in pattern.finditer(code):
            class_name = match.group(1)
            parent_class = match.group(2)
            class_start_pos = match.end()
            # Find end of class by naive closing '};' after class_start_pos
            class_body_end_pos = code.find('};', class_start_pos)
            class_body = code[class_start_pos:class_body_end_pos]
            # Determine actual line numbers of class body range
            start_line_num = code[:class_start_pos].count('\n') + 1
            end_line_num = code[:class_body_end_pos].count('\n') + 1
            # Extract lines only within this class's body
            class_lines = code_lines[start_line_num - 1:end_line_num]
            # Search for ARTIFACT_CLASS() inside this range
            for offset, line in enumerate(class_lines):
                if re.search(r'\bARTIFACT_CLASS\(\);', line):
                    ARTIFACT_class_line = start_line_num + offset
                    results.append({
                        'ClassName': class_name,
                        'ParentClassName': parent_class,
                        'ARTIFACT_CLASS_Line': ARTIFACT_class_line
                    })
                    break
        return results

def generate_class_cpp(dir: str):
    # if dir is not an absolute path, make it absolute by joining with current working directory
    if not os.path.isabs(dir):
        dir = os.path.join(os.getcwd(), dir)

    headers_per_module =  {}
    for modules in os.listdir("./Modules"):
        if os.path.isdir(f"./Modules/{modules}"):
            headers_per_module[modules] = []

    for dirpath, dirnames, filenames in os.walk(dir):
        for filename in filenames:
            if filename.endswith('.h') or filename.endswith('.hpp'):
                results = collect_classes_from_header(f"{dirpath}/{filename}")
                if len(results) > 0:
                    headers_per_module[get_module_name_from_path(dirpath)].append(f"{dirpath}/{filename}")
                    
                    gen_file_path = f"./Build/Intermediate/Classes/{filename.rstrip('hpp').rstrip('.')}.gen.h"
                    os.makedirs(os.path.dirname(gen_file_path), exist_ok=True)
                    with open(gen_file_path, 'w', encoding='utf-8') as gen_file:
                        gen_file.write('#pragma once\n\n')
                        for result in results:
                            if result['ARTIFACT_CLASS_Line'] is not None:
                                print(f'Generating {result["ClassName"]} in {gen_file_path}')
                                gen_file.write(f'''#define _GENERATED_BODY_{result['ARTIFACT_CLASS_Line']} \\
using Super = {result['ParentClassName']};\\
bool IsObjectOfClass(const Class &type) const override {{\\
    return type.Name == "{result['ClassName']}" || Super::IsObjectOfClass(type);\\
}}\\
static Class StaticClass() {{\\
    return Class("{result['ClassName']}");\\
}}\\
struct InternalAlloc\\
{{\\
private:\
    InternalAlloc() = default;\\
    inline static Object::RegisterArtifactClass<{result['ClassName']}> _ClassAllocator_{result['ClassName']};\\
}}/* No ; is appended after this struct, since the macro itself is supposed to have a ; */\\

''')
    for module, headers in headers_per_module.items():
        unique_headers = list(set(headers)) # remove duplicates
        with open(f"./Build/Intermediate/Modules/{module}.gen.cpp", 'w') as gen_module_file:
            gen_module_file.write('#include <vector>\n#include <string>\n\n')
            for header in unique_headers:
                gen_module_file.write(f'#include "{header}"\n')
            gen_module_file.write(f'\nvoid __LinkModule_{module}(std::vector<std::string>& modules) {{\n')
            gen_module_file.write(f'    modules.push_back("{module}");\n')
            gen_module_file.write('}\n')