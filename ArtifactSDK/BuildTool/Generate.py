import os
import json

from BuildTool.Version import VERSION_MAJOR, VERSION_MINOR, get_patch_version
from BuildTool.Platforms import PlatformType
from BuildTool.Util import smart_open

def get_module_json(module_path: str) -> dict:
    with open(f"{module_path}/Module.json", "r") as f:
        return json.load(f)

def generate_cmake(project_path: str, target_platform: str):
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
                
""")
        
        for module in os.listdir(f"{project_path}/Modules"):
            if os.path.isdir(f"{project_path}/Modules/{module}"):
                module_json = get_module_json(f"{project_path}/Modules/{module}")

                # if TargetPlatforms is not specified, assume it's supported on all platforms
                if target_platform not in module_json.get("TargetPlatforms", PlatformType.__members__.keys()):
                    continue

                with smart_open(f"{project_path}/Modules/{module}/CMakeLists.txt") as mf:
                    cpp_src = 'file(GLOB_RECURSE cpp_src "*.cpp")' if module_json.get("SourceDirectories", None) is None else f'file(GLOB_RECURSE cpp_src {" ".join([f"{sd}/*.cpp" for sd in module_json.get("SourceDirectories", [])])})'
                    mf.write(f"""# Generated using Artifact Build Tool for {module}
{cpp_src}
add_library({module} ${{cpp_src}} {project_path}/Build/Intermediate/Modules/{module}.gen.cpp)
target_compile_definitions({module} PRIVATE
    VERSION_MAJOR={VERSION_MAJOR}
    VERSION_MINOR={VERSION_MINOR}
    VERSION_PATCH={get_patch_version()}
)
""")
                    for include_dir in module_json.get("IncludePaths", []):
                        mf.write(f"target_include_directories({module} PUBLIC {include_dir})\n")
                    for import_module in module_json.get("ImportModules", []):
                        import_module_json = get_module_json(f"{project_path}/Modules/{import_module}")
                        for include_dir in import_module_json.get("ExportIncludePaths", []):
                            mf.write(f"target_include_directories({module} PUBLIC {project_path}/Modules/{import_module}/{include_dir})\n")
                        for include_dir in import_module_json.get("IncludePaths", []):
                            mf.write(f"target_include_directories({module} PUBLIC {project_path}/Modules/{import_module}/{include_dir})\n")
                    for additional_project in module_json.get("AddAdditionalCMakeProjects", []):
                        mf.write(f"add_subdirectory({additional_project})\n")


                    mf.write(f"target_include_directories({module} PUBLIC {project_path}/Build/Intermediate/Classes)\n")

                    mf.write(f"""
if(MSVC)
  target_compile_options({module} PRIVATE /W4) # /WX
else()
  target_compile_options({module} PRIVATE -Wall -Wextra -Wpedantic) # -Werror
endif()
""")
                    __LinkModules += f'    extern void __LinkModule_{module}(std::vector<std::string>&); __LinkModule_{module}(modules);\n'

                f.write(f"add_subdirectory(Modules/{module})\n")
                f.write(f"target_link_libraries(Artifact PUBLIC {module})\n")

    __LinkModules += "}\n"
    os.makedirs(f"{project_path}/Build/Intermediate/Modules", exist_ok=True)
    with smart_open(f"{project_path}/Build/Intermediate/Modules/__LinkModules.gen.cpp") as f:
        f.write(__LinkModules)