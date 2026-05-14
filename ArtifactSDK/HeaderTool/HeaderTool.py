import os
import re
import sys
from HeaderTool.Class import Class
from HeaderTool.Struct import Struct
from HeaderTool.Enum import Enum
from BuildTool.Util import get_module_name_from_path, smart_open

class HeaderTool:
    def __init__(self):
        self.headers_per_module = {} # dict[module_name: str, list[absolute_header_file_path: str]]
        self.types_per_header = {} # dict[header_name: str, list[Enum | Struct | Class]]
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
                        if filename.rstrip('hpp').rstrip('.') in self.types_per_header:
                            print(f"Error: Duplicate header name {filename} found at {header_path}. This will cause issues with generated code. Consider renaming one of the headers to have a unique name.")
                            sys.exit(1)
                        self.types_per_header[filename.rstrip('hpp').rstrip('.')] = results


    def generate(self):
        self.reflect_classes()
        for header, types in self.types_per_header.items():
            self.generate_reflection_code(header, types)
        self.generate_module_registration_code()

    @staticmethod
    def collect_classes_from_header(h_file_path: str) -> list[Enum | Struct | Class]:
        with open(h_file_path, 'r', encoding='utf-8') as h_file:
            code = h_file.read()
            code_lines = code.splitlines()
            results = []
            # ARTIFACT_CLASS()
            pattern = re.compile(r'(?:class|struct)\s+(\w+)\s*:\s*public\s+(\w+)\s*\{', re.MULTILINE)
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
                        results.append(Class(class_name, ARTIFACT_class_line, class_body, parent_class))
                        break

            
            # ARTIFACT_STRUCT()
            pattern = re.compile(r'struct\s+(\w+)\s*\{', re.MULTILINE)
            for match in pattern.finditer(code):
                class_name = match.group(1)
                class_start_pos = match.end()
                # Find end of struct by naive closing '};' after class_start_pos
                # TODO: consider same scope and count opening and closing braces to find the correct end of class body
                class_body_end_pos = code.find('};', class_start_pos)
                class_body = code[class_start_pos:class_body_end_pos]
                # Determine actual line numbers of class body range
                start_line_num = code[:class_start_pos].count('\n') + 1
                end_line_num = code[:class_body_end_pos].count('\n') + 1
                # Extract lines only within this class's body
                class_lines = code_lines[start_line_num - 1:end_line_num]
                # Search for ARTIFACT_STRUCT() inside this range
                for offset, line in enumerate(class_lines):
                    if re.search(r'\bARTIFACT_STRUCT\(\);', line):
                        ARTIFACT_struct_line = start_line_num + offset
                        results.append(Struct(class_name, ARTIFACT_struct_line, class_body))
                        break

            # ARTIFACT_ENUM()
            pattern = re.compile(r'enum\s+(class\s+)?(\w+)(?:\s*:\s*([\w:]+))?\s*\{', re.MULTILINE)
            for match in pattern.finditer(code):
                is_enum_class = match.group(1) is not None
                class_name = match.group(2)
                underlying_type = match.group(3) 
                class_start_pos = match.end()
                # Find end of enum by naive closing '};' after class_start_pos
                # TODO: consider same scope and count opening and closing braces to find the correct end of class body
                class_body_end_pos = code.find('};', class_start_pos)
                class_body = code[class_start_pos:class_body_end_pos]
                # Determine actual line numbers of class body range
                start_line_num = code[:class_start_pos].count('\n') + 1
                end_line_num = code[:class_body_end_pos].count('\n') + 1
                # Extract lines only within this class's body
                class_lines = code_lines[start_line_num - 1:end_line_num]
                # ARTIFACT_ENUM() has to be in the line right before start of enum
                if 'ARTIFACT_ENUM' in code_lines[start_line_num - 2]:
                    ARTIFACT_enum_line = start_line_num - 1
                    results.append(Enum(class_name, ARTIFACT_enum_line, class_body, is_enum_class, underlying_type))
            return results

    def reflect_classes(self):
        pass

    def generate_reflection_code(self, header: str, types: list[Enum | Struct | Class]):
        gen_file_path = f"./Build/Intermediate/Classes/{header}.gen.h"
        os.makedirs(os.path.dirname(gen_file_path), exist_ok=True)
        with smart_open(gen_file_path, encoding='utf-8') as gen_file:
            gen_file.write('#pragma once\n\n')
            for result in types:
                if isinstance(result, Enum):
                    print(f'Generating Enum {result.Name} in {gen_file_path}')
                    result.generate_enum_code(gen_file)
                    continue

                if isinstance(result, Struct):
                    print(f'Generating Struct {result.Name} in {gen_file_path}')
                    result.generate_struct_code(gen_file)
                    continue

                if isinstance(result, Class):
                    print(f'Generating Class {result.Name} in {gen_file_path}')
                    result.generate_class_code(gen_file)
                    continue
                    

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