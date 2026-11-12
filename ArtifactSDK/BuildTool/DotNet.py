import json
import os
import shutil
import subprocess
from BuildTool.Module import ArtifactModule
from SDK.Util import smart_open
from SDK.Job import JobError

PROJECT = """<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>{framework}</TargetFramework>
    <AssemblyName>{name}</AssemblyName>
    <RootNamespace>{name}</RootNamespace>
    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>
    <Nullable>disable</Nullable>
    <ImplicitUsings>disable</ImplicitUsings>
    <EnableDynamicLoading>true</EnableDynamicLoading>
    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>
    <GenerateDocumentationFile>false</GenerateDocumentationFile>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
    <OutputPath Condition="'$(Configuration)' == 'Debug'">{editor_output}/</OutputPath>
    <OutputPath Condition="'$(Configuration)' == 'Release'">{package_output}/</OutputPath>
    <NoWarn>CS0108;CS0114</NoWarn>
  </PropertyGroup>
  <ItemGroup>
{sources}
  </ItemGroup>
  <ItemGroup>
{references}
  </ItemGroup>
</Project>
"""

HOST_MODULE = "DotNetScripting"
BINDINGS_ASSEMBLY = "Artifact"


def find_host_module(engine_path: str, project_path: str) -> str:
    for root in (project_path, engine_path):
        module = f"{root}/Modules/{HOST_MODULE}"
        if os.path.exists(f"{module}/Module.json"):
            return module
    return ""


def find_scripting_modules(engine_path: str, project_path: str) -> list[ArtifactModule]:
    modules = {}
    for root in dict.fromkeys([engine_path, project_path]):
        modules_dir = f"{root}/Modules"
        if not os.path.isdir(modules_dir):
            continue
        for name in sorted(os.listdir(modules_dir)):
            if name not in modules and os.path.exists(f"{modules_dir}/{name}/Module.json"):
                module = ArtifactModule.load_from_json(f"{modules_dir}/{name}")
                if module.IsScripting:
                    modules[name] = module
    if BINDINGS_ASSEMBLY in modules:
        raise JobError(f"A scripting module cannot be called '{BINDINGS_ASSEMBLY}', that name belongs to the engine bindings")
    return _dependency_order(modules)


def _dependency_order(modules: dict) -> list[ArtifactModule]:
    ordered = []

    def visit(name: str, chain: list):
        if name in chain:
            raise JobError(f"Scripting modules import each other in a cycle: {' -> '.join(chain + [name])}")
        module = modules.get(name)
        if module is None or module in ordered:
            return
        for dependency in module.ImportModules:
            visit(dependency, chain + [name])
        ordered.append(module)

    for name in modules:
        visit(name, [])
    return ordered


def is_enabled(engine_path: str, project_path: str) -> bool:
    modules = find_scripting_modules(engine_path, project_path)
    if not modules:
        return False
    names = ", ".join(module.name for module in modules)
    if not find_host_module(engine_path, project_path):
        raise JobError(f"The scripting modules {names} need the {HOST_MODULE} module")
    if shutil.which("dotnet") is None:
        raise JobError(f"The .NET SDK (dotnet) is needed to build the scripting modules {names}")
    return True


def intermediate_dir(project_path: str) -> str:
    return f"{project_path}/Build/Intermediate/DotNet"


def editor_output_dir(project_path: str) -> str:
    return f"{project_path}/Binaries/DotNet"


def package_output_dir(project_path: str) -> str:
    return f"{intermediate_dir(project_path)}/Package"


def solution_path(project_path: str) -> str:
    return f"{intermediate_dir(project_path)}/Scripting.slnx"


def _framework() -> str:
    version = subprocess.run(["dotnet", "--version"], capture_output=True, text=True).stdout.strip()
    return f"net{version.split('.')[0]}.0"


def _source_dirs(module: ArtifactModule) -> list[str]:
    directories = module.SourceDirectories or ["."]
    return [os.path.normpath(f"{module.path}/{directory}").replace("\\", "/") for directory in directories]


def _write_project(project_path: str, name: str, sources: list[str], references: list[str], framework: str):
    directory = intermediate_dir(project_path)
    with smart_open(f"{directory}/{name}/{name}.csproj") as file:
        file.write(PROJECT.format(
            framework=framework, name=name,
            editor_output=editor_output_dir(project_path), package_output=package_output_dir(project_path),
            sources="\n".join(f'    <Compile Include="{source}" />' for source in sources),
            references="\n".join(f'    <ProjectReference Include="../{reference}/{reference}.csproj" />' for reference in references)))


def generate_projects(engine_path: str, project_path: str):
    modules = find_scripting_modules(engine_path, project_path)
    framework = _framework()
    directory = intermediate_dir(project_path)
    _write_project(project_path, BINDINGS_ASSEMBLY,
                   [f"{find_host_module(engine_path, project_path)}/Managed/**/*.cs", f"{directory}/Bindings.g.cs"], [], framework)

    scripting = {module.name for module in modules}
    for module in modules:
        references = [BINDINGS_ASSEMBLY] + [name for name in module.ImportModules if name in scripting]
        _write_project(project_path, module.name, [f"{source}/**/*.cs" for source in _source_dirs(module)], references, framework)

    projects = [BINDINGS_ASSEMBLY] + [module.name for module in modules]
    with smart_open(solution_path(project_path)) as file:
        file.write("<Solution>\n" + "".join(f'  <Project Path="{name}/{name}.csproj" />\n' for name in projects) + "</Solution>\n")


def _build(project_path: str, configuration: str):
    command = ["dotnet", "build", solution_path(project_path), "-c", configuration, "--nologo", "-v", "q", "-clp:ErrorsOnly"]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        raise JobError(f"Building the scripting modules failed:\n{result.stdout}{result.stderr}")


def build_for_editor(engine_path: str, project_path: str):
    modules = find_scripting_modules(engine_path, project_path)
    _build(project_path, "Debug")
    manifest = {
        "DotNet": shutil.which("dotnet"),
        "Solution": solution_path(project_path),
        "Assemblies": [module.name for module in modules],
        "Watch": [source for module in modules for source in _source_dirs(module)],
    }
    with smart_open(f"{editor_output_dir(project_path)}/Scripting.json") as file:
        file.write(json.dumps(manifest, indent=4) + "\n")


def build_for_package(engine_path: str, project_path: str) -> list[tuple[str, str]]:
    modules = find_scripting_modules(engine_path, project_path)
    _build(project_path, "Release")
    return [(module.name, f"{package_output_dir(project_path)}/{module.name}.dll") for module in modules]
