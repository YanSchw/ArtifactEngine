import os
import json

def get_module_json(module_path: str) -> dict:
    with open(f"{module_path}/Module.json", "r") as f:
        return json.load(f)

def generate_cmake(project_path: str):
    if project_path == ".":
        project_path = os.getcwd()
    with open(f"{project_path}/CMakeLists.txt", "w") as f:
        f.write(f"""# Generated using Artifact Build Tool
cmake_minimum_required(VERSION 3.5)
project(ArtifactEngine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
                
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${{CMAKE_SOURCE_DIR}}/Binaries")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${{CMAKE_SOURCE_DIR}}/Binaries")
                
add_executable(Artifact {project_path}/Python/empty.cpp)
                
""")
        
        for module in os.listdir(f"{project_path}/Modules"):
            if os.path.isdir(f"{project_path}/Modules/{module}"):
                module_json = get_module_json(f"{project_path}/Modules/{module}")
                with open(f"{project_path}/Modules/{module}/CMakeLists.txt", "w") as mf:
                    cpp_src = 'file(GLOB_RECURSE cpp_src "*.cpp")' if module_json.get("SourceDirectories", None) is None else f'file(GLOB_RECURSE cpp_src {" ".join([f"{sd}/*.cpp" for sd in module_json.get("SourceDirectories", [])])})'
                    mf.write(f"""# Generated using Artifact Build Tool for {module}
{cpp_src}
add_library({module} ${{cpp_src}})
""")
                    for include_dir in module_json.get("IncludePaths", []):
                        mf.write(f"target_include_directories({module} PUBLIC {include_dir})\n")
                    for import_module in module_json.get("ImportModules", []):
                        import_module_json = get_module_json(f"{project_path}/Modules/{import_module}")
                        for include_dir in import_module_json.get("ExportIncludePaths", []):
                            mf.write(f"target_include_directories({module} PUBLIC {project_path}/Modules/{import_module}/{include_dir})\n")
                    for additional_project in module_json.get("AddAdditionalCMakeProjects", []):
                        mf.write(f"add_subdirectory({additional_project})\n")


                    mf.write(f"target_include_directories({module} PUBLIC {project_path}/Build/Intermediate/Classes)\n")

                f.write(f"add_subdirectory(Modules/{module})\n")
                f.write(f"target_link_libraries(Artifact PUBLIC {module})\n")