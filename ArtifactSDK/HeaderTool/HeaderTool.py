import os
import re
import sys
from BuildTool.Util import get_module_name_from_path, smart_open

class HeaderTool:
    def __init__(self):
        self.headers_per_module = {} # dict[module_name: str, list[absolute_header_file_path: str]]
        self.classes_per_header = {} # dict[header_name: str, list[dict]]
        self.visited_header = set()  # set of absolute header file paths to avoid processing the same header multiple times

    def collect_headers(self, dir):
        # if dir is not an absolute path, make it absolute by joining with current working directory
        if not os.path.isabs(dir):
            dir = os.path.join(os.getcwd(), dir)

        # Initialize modules from the Modules directory to ensure all modules are registered, even those without classes or headers.
        # This allows for empty modules to be reflected and registered properly.
        for module in os.listdir("./Modules"):
            if os.path.isdir(f"./Modules/{module}") and module not in self.headers_per_module:
                self.headers_per_module[module] = []

        for dirpath, dirnames, filenames in os.walk(dir):
            for filename in filenames:
                if filename.endswith('.h') or filename.endswith('.hpp'):
                    header_path = f"{dirpath}/{filename}"
                    if header_path in self.visited_header:
                        continue
                    self.visited_header.add(header_path)

                    results = HeaderTool.collect_classes_from_header(header_path)
                    if len(results) > 0:
                        self.headers_per_module[get_module_name_from_path(dirpath)].append(header_path)
                        if filename.rstrip('hpp').rstrip('.') in self.classes_per_header:
                            print(f"Error: Duplicate header name {filename} found at {header_path}. This will cause issues with generated code. Consider renaming one of the headers to have a unique name.")
                            sys.exit(1)
                        self.classes_per_header[filename.rstrip('hpp').rstrip('.')] = results


    def generate(self):
        self.reflect_classes()
        for header, classes in self.classes_per_header.items():
            self.generate_reflection_code(header, classes)
        self.generate_module_registration_code()

    @staticmethod
    def collect_classes_from_header(h_file_path: str) -> list[dict]:
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
                # TODO: consider same scope and count opening and closing braces to find the correct end of class body
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
                            'HeaderFilePath': h_file_path,
                            'ClassBody': class_body,
                            'ARTIFACT_CLASS_Line': ARTIFACT_class_line
                        })
                        break
            return results

    def reflect_classes(self):
        pass

    def generate_reflection_code(self, header: str, classes: list[dict]):
        gen_file_path = f"./Build/Intermediate/Classes/{header}.gen.h"
        os.makedirs(os.path.dirname(gen_file_path), exist_ok=True)
        with smart_open(gen_file_path, encoding='utf-8') as gen_file:
            gen_file.write('#pragma once\n\n')
            for result in classes:
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

    def generate_module_registration_code(self):
        for module, headers in self.headers_per_module.items():
            unique_headers = list(set(headers)) # remove duplicates
            with smart_open(f"./Build/Intermediate/Modules/{module}.gen.cpp") as gen_module_file:
                gen_module_file.write('#include <vector>\n#include <string>\n\n')
                for header in unique_headers:
                    gen_module_file.write(f'#include "{header}"\n')
                gen_module_file.write(f'\nvoid __LinkModule_{module}(std::vector<std::string>& modules) {{\n')
                gen_module_file.write(f'    modules.push_back("{module}");\n')
                gen_module_file.write('}\n')