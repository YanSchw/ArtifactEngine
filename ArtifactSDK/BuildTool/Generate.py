import os
import json

from SDK.Version import VERSION_MAJOR, VERSION_MINOR, get_patch_version
from SDK.Platforms import PlatformType, get_current_platform, get_cpp_platform_macro
from SDK.Paths import get_engine_path
from BuildTool.Target import TargetType, get_cpp_target_macro
from BuildTool.Module import ArtifactModule
from SDK.Util import smart_open

def discover_modules(engine_path: str, project_path: str, target_platform: str):
    """Every module the build should include, resolved across the engine and the project.

    Returns a list of (name, module_dir, ArtifactModule, owning_root). Engine modules come
    first; a project module with the same name as an engine one is not overridden (engine-first). 
    `owning_root` is the engine or project directory the module lives under."""
    engine_path = engine_path.replace("\\", "/").rstrip("/")
    project_path = project_path.replace("\\", "/").rstrip("/")
    same_repo = os.path.normcase(os.path.normpath(engine_path)) == os.path.normcase(os.path.normpath(project_path))

    modules = []
    seen = set()

    def add(modules_dir: str, owning_root: str):
        if not os.path.isdir(modules_dir):
            return
        for name in sorted(os.listdir(modules_dir)):
            module_dir = f"{modules_dir}/{name}"
            if not os.path.isdir(module_dir) or not os.path.exists(f"{module_dir}/Module.json"):
                continue
            if name in seen:
                continue
            module = ArtifactModule.load_from_json(module_dir)
            if not module.supports_platform(target_platform):
                continue
            seen.add(name)
            modules.append((name, module_dir, module, owning_root))

    add(f"{engine_path}/Modules", engine_path)
    if not same_repo:
        add(f"{project_path}/Modules", project_path)

    return modules

def expand_indirect_module_dependencies(module_dirs: dict[str, str], import_modules: list[str]) -> set[str]:
    to_check = set(import_modules)
    expanded = set()

    while to_check:
        module_name = to_check.pop()
        # skip if already processed
        if module_name in expanded:
            continue

        expanded.add(module_name)
        module_dir = module_dirs.get(module_name)
        if module_dir is None:
            continue
        module = ArtifactModule.load_from_json(module_dir)
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
    project_path = project_path.replace("\\", "/").rstrip("/")

    engine_path = get_engine_path().replace("\\", "/").rstrip("/")
    modules = discover_modules(engine_path, project_path, target_platform)
    module_dirs = {name: module_dir for name, module_dir, _, _ in modules}

    global_definitions = [get_cpp_platform_macro(get_current_platform()), get_cpp_target_macro(target_configuration)]
    if is_packaged:
        global_definitions.append("AE_PACKAGED")

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

add_compile_definitions({" ".join(global_definitions)})

add_executable(Artifact {project_path}/Build/Intermediate/Modules/__LinkModules.gen.cpp)

if (WIN32)
    set(APP_ICON_RESOURCE "{project_path}/Build/Intermediate/Resources/Win64IconResource.rc")
    target_sources(Artifact PRIVATE ${{APP_ICON_RESOURCE}})
endif()

""")
        
        for module_name, module_dir, module, owning_root in modules:
            with smart_open(f"{module_dir}/CMakeLists.txt") as mf:
                cpp_src = 'file(GLOB_RECURSE cpp_src "*.cpp")' if module.SourceDirectories is None else f'file(GLOB_RECURSE cpp_src {" ".join(module.get_source_files_pattern())})'
                mf.write(f"""# Generated using Artifact Build Tool for {module_name}
{cpp_src}
add_library({module_name} ${{cpp_src}} {owning_root}/Build/Intermediate/Modules/{module_name}.gen.cpp)
""")
                for include_dir in module.IncludePaths:
                    mf.write(f"target_include_directories({module_name} PUBLIC {include_dir})\n")
                for import_module_name in sorted(expand_indirect_module_dependencies(module_dirs, module.ImportModules)):
                    import_module_dir = module_dirs.get(import_module_name)
                    if import_module_dir is None:
                        continue
                    import_module = ArtifactModule.load_from_json(import_module_dir)
                    for include_dir in import_module.ExportIncludePaths:
                        mf.write(f"target_include_directories({module_name} PUBLIC {import_module_dir}/{include_dir})\n")
                    for include_dir in import_module.IncludePaths:
                        mf.write(f"target_include_directories({module_name} PUBLIC {import_module_dir}/{include_dir})\n")
                for additional_project in module.AddAdditionalCMakeProjects:
                    mf.write(f"add_subdirectory({additional_project})\n")


                for classes_root in dict.fromkeys([owning_root, engine_path]):
                    mf.write(f"target_include_directories({module_name} PUBLIC {classes_root}/Build/Intermediate/Classes)\n")

                mf.write(f"""
if(MSVC)
  target_compile_options({module_name} PRIVATE /W4) # /WX
else()
  target_compile_options({module_name} PRIVATE -Wall -Wextra -Wpedantic) # -Werror
endif()
""")
                __LinkModules += f'    extern void __LinkModule_{module_name}(std::vector<std::string>&); __LinkModule_{module_name}(modules);\n'

            # Engine modules live outside the project tree, so add_subdirectory needs an explicit binary dir.
            if module_dir.startswith(f"{project_path}/"):
                f.write(f"add_subdirectory({module_dir[len(project_path) + 1:]})\n")
            else:
                f.write(f"add_subdirectory({module_dir} {project_path}/Build/Intermediate/ModuleBuild/{module_name})\n")
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