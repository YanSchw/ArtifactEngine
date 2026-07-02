import os
import json

from SDK.Version import VERSION_MAJOR, VERSION_MINOR, get_patch_version
from SDK.Platforms import PlatformType, get_current_platform, get_cpp_platform_macro
from BuildTool.Target import TargetType, get_cpp_target_macro
from BuildTool.Module import ArtifactModule
from SDK.Util import smart_open

def expand_indirect_module_dependencies(project_path: str, import_modules: list[str]) -> set[str]:
    to_check = set(import_modules)
    expanded = set()

    while to_check:
        module_name = to_check.pop()
        # skip if already processed
        if module_name in expanded:
            continue

        expanded.add(module_name)
        module = ArtifactModule.load_from_json(f"{project_path}/Modules/{module_name}")
        for dep in module.ImportModules:
            if dep not in expanded:
                to_check.add(dep)

    return expanded

def generate_cmake(project_path: str, args):
    target_platform = args.target
    target_configuration = args.configuration
    is_packaged = args.packaged if hasattr(args, "packaged") else False
    if project_path == ".":
        project_path = os.getcwd()

    __LinkModules = """#include <vector>
#include <string>

void __LinkModules(std::vector<std::string>& modules) {
"""

    with smart_open(f"{project_path}/CMakeLists.txt") as f:
        f.write(f"""# Generated using Artifact Build Tool
cmake_minimum_required(VERSION 3.5)
project(ArtifactEngine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
                
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${{CMAKE_SOURCE_DIR}}/Binaries")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${{CMAKE_SOURCE_DIR}}/Binaries")
                
if (MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(
        _UNICODE
        UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()
                
add_executable(Artifact {project_path}/Build/Intermediate/Modules/__LinkModules.gen.cpp)

if (WIN32)
    set(APP_ICON_RESOURCE "{project_path}/Build/Intermediate/Resources/Win64IconResource.rc")
    target_sources(Artifact PRIVATE ${{APP_ICON_RESOURCE}})
endif()

""")
        
        for module_name in sorted(os.listdir(f"{project_path}/Modules")):
            module_dir = f"{project_path}/Modules/{module_name}"
            if os.path.isdir(module_dir):
                module = ArtifactModule.load_from_json(module_dir)

                # if TargetPlatforms is not specified, assume it's supported on all platforms
                if not module.supports_platform(target_platform):
                    continue

                with smart_open(f"{project_path}/Modules/{module_name}/CMakeLists.txt") as mf:
                    cpp_src = 'file(GLOB_RECURSE cpp_src "*.cpp")' if module.SourceDirectories is None else f'file(GLOB_RECURSE cpp_src {" ".join(module.get_source_files_pattern())})'
                    mf.write(f"""# Generated using Artifact Build Tool for {module_name}
{cpp_src}
add_library({module_name} ${{cpp_src}} {project_path}/Build/Intermediate/Modules/{module_name}.gen.cpp)
""")
                    mf.write(f"target_compile_definitions({module_name} PUBLIC {get_cpp_platform_macro(get_current_platform())})\n")
                    mf.write(f"target_compile_definitions({module_name} PUBLIC {get_cpp_target_macro(target_configuration)})\n")
                    if is_packaged:
                        mf.write(f"target_compile_definitions({module_name} PUBLIC AE_PACKAGED)\n")
                    for include_dir in module.IncludePaths:
                        mf.write(f"target_include_directories({module_name} PUBLIC {include_dir})\n")
                    for import_module_name in sorted(expand_indirect_module_dependencies(project_path, module.ImportModules)):
                        import_module = ArtifactModule.load_from_json(f"{project_path}/Modules/{import_module_name}")
                        for include_dir in import_module.ExportIncludePaths:
                            mf.write(f"target_include_directories({module_name} PUBLIC {project_path}/Modules/{import_module_name}/{include_dir})\n")
                        for include_dir in import_module.IncludePaths:
                            mf.write(f"target_include_directories({module_name} PUBLIC {project_path}/Modules/{import_module_name}/{include_dir})\n")
                    for additional_project in module.AddAdditionalCMakeProjects:
                        mf.write(f"add_subdirectory({additional_project})\n")


                    mf.write(f"target_include_directories({module_name} PUBLIC {project_path}/Build/Intermediate/Classes)\n")

                    mf.write(f"""
if(MSVC)
  target_compile_options({module_name} PRIVATE /W4) # /WX
else()
  target_compile_options({module_name} PRIVATE -Wall -Wextra -Wpedantic) # -Werror
endif()
""")
                    __LinkModules += f'    extern void __LinkModule_{module_name}(std::vector<std::string>&); __LinkModule_{module_name}(modules);\n'

                f.write(f"add_subdirectory(Modules/{module_name})\n")
                f.write(f"target_link_libraries(Artifact PUBLIC {module_name})\n")

    __LinkModules += "}\n"
    os.makedirs(f"{project_path}/Build/Intermediate/Modules", exist_ok=True)
    with smart_open(f"{project_path}/Build/Intermediate/Modules/__LinkModules.gen.cpp") as f:
        f.write(__LinkModules)
        f.write(f"""

int __VERSION_MAJOR() {{ return {VERSION_MAJOR}; }}
int __VERSION_MINOR() {{ return {VERSION_MINOR}; }}
int __VERSION_PATCH() {{ return {get_patch_version() if get_patch_version() is not None else '-1'}; }}
""")