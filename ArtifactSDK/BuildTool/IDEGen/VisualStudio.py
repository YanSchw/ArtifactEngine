import os
import sys
import uuid
from SDK.Util import smart_open

# .sln project-type GUID for all .vcxproj entries (Makefile or not).
_VCXPROJ_TYPE_GUID = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}"
_SOLUTION_FOLDER_GUID = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}"

# Matches BuildTool.Target.TargetType, so "--configuration $(Configuration)" lines up.
_CONFIGURATIONS = ["Debug", "Dev", "Dist"]
_PLATFORM = "x64"

_CONFIG_DEFINES = {
    "Debug": "AE_TARGET_DEBUG",
    "Dev": "AE_TARGET_DEV",
    "Dist": "AE_TARGET_DIST",
}

_COMMON_DEFINES = ["AE_PLATFORM_WIN64", "_UNICODE", "UNICODE", "NOMINMAX", "WIN32_LEAN_AND_MEAN"]


def _guid(seed: str) -> str:
    return "{" + str(uuid.uuid5(uuid.NAMESPACE_DNS, seed)).upper() + "}"


def _xml_text(s: str) -> str:
    """Escape a string for use as XML element content (e.g. `&&` in a shell command)."""
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def _xml_attr(s: str) -> str:
    """Escape a string for use as an XML attribute value."""
    return _xml_text(s).replace('"', "&quot;")


def _venv_activate_script() -> str:
    """Path to the venv currently running the SDK."""
    return f"{sys.prefix.replace(chr(92), '/')}/Scripts/activate.bat"


def _build_command(activate_bat: str, target: str, extra_args: str = "") -> str:
    return f'call "{activate_bat}" && artifact build --configuration $(Configuration) --target {target}{extra_args}'


def _clean_command(project_path: str) -> str:
    win_path = project_path.replace("/", "\\")
    return (
        f'if exist "{win_path}\\Build" rmdir /s /q "{win_path}\\Build" & '
        f'if exist "{win_path}\\Binaries" rmdir /s /q "{win_path}\\Binaries"'
    )


def _config_property_group(configuration: str, is_startup: bool) -> str:
    defines = ";".join(_COMMON_DEFINES + [_CONFIG_DEFINES[configuration]])
    condition = f"'$(Configuration)|$(Platform)'=='{configuration}|{_PLATFORM}'"
    lines = [f'  <PropertyGroup Condition="{condition}">']
    lines.append(f"    <NMakePreprocessorDefinitions>{defines};$(NMakePreprocessorDefinitions)</NMakePreprocessorDefinitions>")
    if is_startup:
        lines.append("    <LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory>")
        lines.append("    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>")
    lines.append("  </PropertyGroup>")
    return "\n".join(lines)


def _vcxproj_xml(name: str, guid: str, project_path: str, activate_bat: str,
                  source_files: list[str], header_files: list[str], other_files: list[str],
                  include_dirs: list[str], output_exe: str, is_startup: bool) -> str:
    project_configs = "\n".join(
        f"""    <ProjectConfiguration Include="{c}|{_PLATFORM}">
      <Configuration>{c}</Configuration>
      <Platform>{_PLATFORM}</Platform>
    </ProjectConfiguration>"""
        for c in _CONFIGURATIONS
    )

    config_type_groups = "\n".join(
        f"""  <PropertyGroup Label="Configuration" Condition="'$(Configuration)|$(Platform)'=='{c}|{_PLATFORM}'">
    <ConfigurationType>Makefile</ConfigurationType>
    <PlatformToolset>v143</PlatformToolset>
    <UseDebugLibraries>{str(c == 'Debug').lower()}</UseDebugLibraries>
  </PropertyGroup>"""
        for c in _CONFIGURATIONS
    )

    nmake_groups = "\n".join(
        f"""  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='{c}|{_PLATFORM}'">
    <NMakeBuildCommandLine>{_xml_text(_build_command(activate_bat, "Win64"))}</NMakeBuildCommandLine>
    <NMakeReBuildCommandLine>{_xml_text(_build_command(activate_bat, "Win64", " --clean"))}</NMakeReBuildCommandLine>
    <NMakeCleanCommandLine>{_xml_text(_clean_command(project_path))}</NMakeCleanCommandLine>
    <NMakeOutput>{_xml_text(output_exe)}</NMakeOutput>
    <NMakeIncludeSearchPath>{_xml_text(";".join(include_dirs))}</NMakeIncludeSearchPath>
    <LanguageStandard>stdcpp20</LanguageStandard>
    <AdditionalOptions>/std:c++20 /Zc:__cplusplus %(AdditionalOptions)</AdditionalOptions>
  </PropertyGroup>"""
        for c in _CONFIGURATIONS
    )

    defines_groups = "\n".join(_config_property_group(c, is_startup) for c in _CONFIGURATIONS)

    cl_compile = "\n".join(f'    <ClCompile Include="{_xml_attr(f)}" />' for f in source_files)
    cl_include = "\n".join(f'    <ClInclude Include="{_xml_attr(f)}" />' for f in header_files)
    none_items = "\n".join(f'    <None Include="{_xml_attr(f)}" />' for f in other_files)

    return f"""<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="17.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
{project_configs}
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCProjectVersion>17.0</VCProjectVersion>
    <ProjectGuid>{guid}</ProjectGuid>
    <Keyword>MakeFileProj</Keyword>
    <ProjectName>{name}</ProjectName>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
{config_type_groups}
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />
{nmake_groups}
{defines_groups}
  <ItemGroup>
{cl_compile}
{cl_include}
{none_items}
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />
</Project>
"""


def _write_filters(vcxproj_path: str, source_files: list[str], header_files: list[str],
                    other_files: list[str], module_path: str):
    """A .vcxproj.filters file that mirrors on-disk subfolders in the Solution Explorer tree."""

    def rel_filter(path: str) -> str | None:
        rel = os.path.relpath(path, module_path).replace("\\", "/")
        folder = "/".join(rel.split("/")[:-1])
        return folder or None

    folders = set()
    for f in source_files + header_files + other_files:
        folder = rel_filter(f)
        while folder:
            folders.add(folder)
            folder = "/".join(folder.split("/")[:-1])

    filter_items = "\n".join(
        f"""    <Filter Include="{_xml_attr(folder)}">
      <UniqueIdentifier>{_guid('artifact-vs-filter-' + module_path + '-' + folder)}</UniqueIdentifier>
    </Filter>"""
        for folder in sorted(folders)
    )

    def file_items(files, tag):
        items = []
        for f in files:
            folder = rel_filter(f)
            if folder:
                items.append(f'    <{tag} Include="{_xml_attr(f)}"><Filter>{_xml_text(folder)}</Filter></{tag}>')
            else:
                items.append(f'    <{tag} Include="{_xml_attr(f)}" />')
        return "\n".join(items)

    content = f"""<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
{filter_items}
  </ItemGroup>
  <ItemGroup>
{file_items(source_files, "ClCompile")}
  </ItemGroup>
  <ItemGroup>
{file_items(header_files, "ClInclude")}
  </ItemGroup>
  <ItemGroup>
{file_items(other_files, "None")}
  </ItemGroup>
</Project>
"""
    with smart_open(f"{vcxproj_path}.filters") as f:
        f.write(content)


def _write_solution(project_path: str, solution_name: str, artifact_guid: str, artifact_vcxproj: str,
                     engine_folder_guid: str, project_folder_guid: str,
                     engine_modules, project_modules):
    lines = []
    lines.append("Microsoft Visual Studio Solution File, Format Version 12.00")
    lines.append("# Visual Studio Version 17")
    lines.append("VisualStudioVersion = 17.0.31903.59")
    lines.append("MinimumVisualStudioVersion = 10.0.40219.1")

    # Emitted first: with no pre-existing .suo, VS defaults the startup project to the first one listed.
    lines.append(f'Project("{_VCXPROJ_TYPE_GUID}") = "Artifact", "{artifact_vcxproj}", "{artifact_guid}"')
    lines.append("EndProject")

    lines.append(f'Project("{_SOLUTION_FOLDER_GUID}") = "Project", "Project", "{project_folder_guid}"')
    lines.append("EndProject")

    if engine_modules:
        lines.append(f'Project("{_SOLUTION_FOLDER_GUID}") = "Engine", "Engine", "{engine_folder_guid}"')
        lines.append("EndProject")

    for module in engine_modules + project_modules:
        vcxproj = f"{module.path}/{module.name}.vcxproj"
        lines.append(f'Project("{_VCXPROJ_TYPE_GUID}") = "{module.name}", "{vcxproj}", "{module.guid}"')
        lines.append("EndProject")

    lines.append("Global")
    lines.append("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution")
    for c in _CONFIGURATIONS:
        lines.append(f"\t\t{c}|{_PLATFORM} = {c}|{_PLATFORM}")
    lines.append("\tEndGlobalSection")

    lines.append("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution")
    for c in _CONFIGURATIONS:
        lines.append(f"\t\t{artifact_guid}.{c}|{_PLATFORM}.ActiveCfg = {c}|{_PLATFORM}")
        lines.append(f"\t\t{artifact_guid}.{c}|{_PLATFORM}.Build.0 = {c}|{_PLATFORM}")
    for module in engine_modules + project_modules:
        for c in _CONFIGURATIONS:
            # No Build.0 entry: "Build Solution" only invokes Artifact, not every module project.
            lines.append(f"\t\t{module.guid}.{c}|{_PLATFORM}.ActiveCfg = {c}|{_PLATFORM}")
    lines.append("\tEndGlobalSection")

    lines.append("\tGlobalSection(SolutionProperties) = preSolution")
    lines.append("\t\tHideSolutionNode = FALSE")
    lines.append("\tEndGlobalSection")

    lines.append("\tGlobalSection(NestedProjects) = preSolution")
    lines.append(f"\t\t{artifact_guid} = {project_folder_guid}")
    for module in engine_modules:
        lines.append(f"\t\t{module.guid} = {engine_folder_guid}")
    for module in project_modules:
        lines.append(f"\t\t{module.guid} = {project_folder_guid}")
    lines.append("\tEndGlobalSection")

    lines.append("\tGlobalSection(ExtensibilityGlobals) = postSolution")
    lines.append(f"\t\tSolutionGuid = {_guid('artifact-vs-solution-' + project_path)}")
    lines.append("\tEndGlobalSection")
    lines.append("EndGlobal")
    lines.append("")

    with smart_open(f"{project_path}/{solution_name}.sln") as f:
        f.write("\n".join(lines))


def generate_visual_studio(engine_path: str, project_path: str, modules, args):
    solution_name = (project_path.rstrip("/").rsplit("/", 1)[-1]) or "Artifact"
    activate_bat = _venv_activate_script()
    output_exe = f"{project_path}/Binaries/Artifact.exe"

    artifact_guid = _guid("artifact-vs-project-Artifact")
    engine_folder_guid = _guid("artifact-vs-folder-Engine")
    project_folder_guid = _guid("artifact-vs-folder-Project")

    for module in modules:
        module.guid = _guid(f"artifact-vs-project-{module.name}")
        vcxproj_path = f"{module.path}/{module.name}.vcxproj"
        with smart_open(vcxproj_path) as f:
            f.write(_vcxproj_xml(
                name=module.name,
                guid=module.guid,
                project_path=project_path,
                activate_bat=activate_bat,
                source_files=module.source_files,
                header_files=module.header_files,
                other_files=module.other_files,
                include_dirs=module.include_dirs,
                output_exe=output_exe,
                is_startup=False,
            ))
        _write_filters(vcxproj_path, module.source_files, module.header_files, module.other_files, module.path)

    artifact_vcxproj = f"{project_path}/Artifact.vcxproj"
    with smart_open(artifact_vcxproj) as f:
        f.write(_vcxproj_xml(
            name="Artifact",
            guid=artifact_guid,
            project_path=project_path,
            activate_bat=activate_bat,
            source_files=[],
            header_files=[],
            other_files=[],
            include_dirs=[],
            output_exe=output_exe,
            is_startup=True,
        ))

    engine_modules = [m for m in modules if m.category == "Engine"]
    project_modules = [m for m in modules if m.category == "Project"]

    _write_solution(project_path, solution_name, artifact_guid, artifact_vcxproj,
                     engine_folder_guid, project_folder_guid, engine_modules, project_modules)
