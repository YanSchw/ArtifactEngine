import os

def generate_cmake(project_path: str):
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
                with open(f"{project_path}/Modules/{module}/CMakeLists.txt", "w") as mf:
                    mf.write(f"""# Generated using Artifact Build Tool for {module}
file(GLOB_RECURSE cpp_src "*.cpp")
add_library({module} ${{cpp_src}})
""")
                f.write(f"add_subdirectory(Modules/{module})\n")
                f.write(f"target_link_libraries(Artifact PUBLIC {module})\n")