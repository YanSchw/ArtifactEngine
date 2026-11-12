import os
import re
import sys
from HeaderTool.Class import Class
from HeaderTool.Struct import Struct
from HeaderTool.Enum import Enum
from HeaderTool.Properties import HeaderToolError
from SDK.Util import get_module_name_from_path, smart_open
from BuildTool.Module import ArtifactModule

def _code_chars(code: str, start: int):
    """Yields (index, char) for every character of code from start on that is outside comments and literals."""
    i = start
    n = len(code)
    while i < n:
        c = code[i]
        if code.startswith('//', i):
            end = code.find('\n', i)
            i = n if end < 0 else end
            continue
        if code.startswith('/*', i):
            end = code.find('*/', i + 2)
            i = n if end < 0 else end + 2
            continue
        if c == '"' or c == "'":
            j = i + 1
            while j < n and code[j] != c and code[j] != '\n':
                j += 2 if code[j] == '\\' else 1
            i = j + 1
            continue
        yield i, c
        i += 1


def _find_scope_end(code: str, open_pos: int) -> int:
    """Index of the '}' closing the scope whose '{' sits right before open_pos."""
    depth = 1
    for i, c in _code_chars(code, open_pos):
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i
    return code.find('};', open_pos)


def _top_level(body: str) -> str:
    """body with every nested scope blanked out (newlines kept)."""
    code_positions = {i for i, _ in _code_chars(body, 0)}
    result = []
    depth = 0
    for i, c in enumerate(body):
        is_code = i in code_positions
        if is_code and c == '}':
            depth -= 1
        result.append(' ' if depth > 0 and c != '\n' else c)
        if is_code and c == '{':
            depth += 1
    return ''.join(result)


def _find_marker_line(body: str, start_line_num: int, marker: str) -> int:
    match = re.search(r'\b' + marker + r'\(\);', body)
    return -1 if match is None else start_line_num + body[:match.start()].count('\n')


class HeaderTool:
    def __init__(self, target_platform: str):
        self.target_platform = target_platform
        self.headers_per_module = {} # dict[module_name: str, list[absolute_header_file_path: str]]
        self.types_per_header = {} # dict[header_name: str, list[Enum | Struct | Class]]
        self.visited_header = set()  # set of absolute header file paths to avoid processing the same header multiple times
        self.module_output_root = {}  # dict[module_name: str, owning_root_dir: str]
        self.header_output_root = {}  # dict[header_name: str, owning_root_dir: str]
        self.header_paths = {}
        self.editor_only_modules = set()
        self.skipped_dirs = set()  # absolute dirs belonging to another target platform

    def _skips(self, dirpath: str) -> bool:
        dirpath = os.path.normpath(dirpath)
        return any(dirpath == skipped or dirpath.startswith(skipped + os.sep) for skipped in self.skipped_dirs)

    def collect_headers(self, dir, output_root):
        # if dir is not an absolute path, make it absolute by joining with current working directory
        if not os.path.isabs(dir):
            dir = os.path.join(os.getcwd(), dir)
        output_root = output_root.replace("\\", "/").rstrip("/")

        # Register every module the target platform builds, so class-less modules are still
        # reflected. A module (or a source directory) belonging to another platform is not compiled,
        # and its headers would not compile here either, so it is left out entirely.
        for module in os.listdir(dir):
            module_dir = f"{dir}/{module}"
            if not os.path.isdir(module_dir):
                continue

            if os.path.exists(f"{module_dir}/Module.json"):
                metadata = ArtifactModule.load_from_json(module_dir)
                if metadata.IsScripting:
                    self.skipped_dirs.add(os.path.normpath(module_dir))
                    continue
                if metadata.IsNonPackageOnly:
                    self.editor_only_modules.add(module)
                if not metadata.supports_platform(self.target_platform):
                    self.skipped_dirs.add(os.path.normpath(module_dir))
                    continue
                for foreign in metadata.get_foreign_source_directories(self.target_platform):
                    self.skipped_dirs.add(os.path.normpath(f"{module_dir}/{foreign}"))

            if module not in self.headers_per_module:
                self.headers_per_module[module] = []
                self.module_output_root[module] = output_root

        for dirpath, dirnames, filenames in os.walk(dir):
            if self._skips(dirpath):
                dirnames[:] = []
                continue
            for filename in filenames:
                if filename.endswith('.h') or filename.endswith('.hpp'):
                    header_path = f"{dirpath}/{filename}"
                    if header_path in self.visited_header:
                        continue
                    self.visited_header.add(header_path)

                    module_name = get_module_name_from_path(dirpath)
                    results = HeaderTool.collect_classes_from_header(header_path, module_name)
                    if len(results) > 0:
                        self.headers_per_module[module_name].append(header_path)
                        header_name = filename.rstrip('hpp').rstrip('.')
                        if header_name in self.types_per_header:
                            print(f"Error: Duplicate header name {filename} found at {header_path}. This will cause issues with generated code. Consider renaming one of the headers to have a unique name.")
                            sys.exit(1)
                        self.types_per_header[header_name] = results
                        self.header_paths[header_name] = header_path
                        self.header_output_root[header_name] = output_root


    def generate(self):
        self.reflect_classes()
        for header, types in self.types_per_header.items():
            try:
                self.generate_reflection_code(header, types)
            except HeaderToolError as error:
                print(f"{self.header_paths.get(header, header)}: error: {error}")
                sys.exit(1)
        self.generate_module_registration_code()

    @staticmethod
    def collect_classes_from_header(h_file_path: str, module_name: str) -> list[Enum | Struct | Class]:
        with open(h_file_path, 'r', encoding='utf-8', errors='ignore') as h_file:
            code = h_file.read()
            code_lines = code.splitlines()
            results = []
            # ARTIFACT_CLASS()
            pattern = re.compile(r'(?:class|struct)\s+(\w+)\s*(?:final\s*)?:\s*public\s+(\w+)\s*\{', re.MULTILINE)
            for match in pattern.finditer(code):
                class_name = match.group(1)
                parent_class = match.group(2)
                class_start_pos = match.end()
                class_body = _top_level(code[class_start_pos:_find_scope_end(code, class_start_pos)])
                start_line_num = code[:class_start_pos].count('\n') + 1
                ARTIFACT_class_line = _find_marker_line(class_body, start_line_num, 'ARTIFACT_CLASS')
                if ARTIFACT_class_line >= 0:
                    results.append(Class(class_name, ARTIFACT_class_line, class_body, parent_class, module_name))

            
            # ARTIFACT_STRUCT()
            pattern = re.compile(r'struct\s+(\w+)\s*\{', re.MULTILINE)
            for match in pattern.finditer(code):
                class_name = match.group(1)
                class_start_pos = match.end()
                class_body = _top_level(code[class_start_pos:_find_scope_end(code, class_start_pos)])
                start_line_num = code[:class_start_pos].count('\n') + 1
                ARTIFACT_struct_line = _find_marker_line(class_body, start_line_num, 'ARTIFACT_STRUCT')
                if ARTIFACT_struct_line >= 0:
                    results.append(Struct(class_name, ARTIFACT_struct_line, class_body))

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
        output_root = self.header_output_root.get(header, ".")
        gen_file_path = f"{output_root}/Build/Intermediate/Classes/{header}.gen.h"
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
            unique_headers = sorted(set(headers)) # remove duplicates; sorted so the generated #include order is stable across runs (avoids needless recompiles)
            output_root = self.module_output_root.get(module, ".")
            with smart_open(f"{output_root}/Build/Intermediate/Modules/{module}.gen.cpp") as gen_module_file:
                gen_module_file.write('#include <vector>\n#include <string>\n\n')
                for header in unique_headers:
                    gen_module_file.write(f'#include "{header}"\n')
                gen_module_file.write(f'\nvoid __LinkModule_{module}(std::vector<std::string>& modules) {{\n')
                gen_module_file.write(f'    modules.push_back("{module}");\n')
                gen_module_file.write('}\n')