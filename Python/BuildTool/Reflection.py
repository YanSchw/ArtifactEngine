import os
import re

def generate_class_cpp(dir: str):
    for dirpath, dirnames, filenames in os.walk(dir):
        for filename in filenames:
            if filename.endswith('.h') or filename.endswith('.hpp'):
                with open(f'{dirpath}/{filename}', 'r') as h_file:
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

                    if len(results) > 0:
                        gen_file_path = f"./Build/Intermediate/Classes/{filename.rstrip('hpp').rstrip('.')}.gen.h"
                        os.makedirs(os.path.dirname(gen_file_path), exist_ok=True)
                        with open(gen_file_path, 'w') as gen_file:
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
}};\\

''')