import hashlib
import os
import sys
from SDK.Util import smart_open

# Matches BuildTool.Target.TargetType; $CONFIGURATION is the env var Xcode
# exports for the legacy target's build script.
_CONFIGURATIONS = ["Debug", "Dev", "Dist"]

_FILE_TYPES = {
    ".cpp": "sourcecode.cpp.cpp",
    ".cc": "sourcecode.cpp.cpp",
    ".cxx": "sourcecode.cpp.cpp",
    ".h": "sourcecode.c.h",
    ".hpp": "sourcecode.c.h",
    ".inl": "sourcecode.c.h",
}


def _oid(seed: str) -> str:
    """A deterministic 24-hex-char Xcode-style object id."""
    return hashlib.sha1(seed.encode("utf-8")).hexdigest()[:24].upper()


def _pbx_str(s: str) -> str:
    """Quote a plist string only if it needs it (Xcode's convention)."""
    if s and all(c.isalnum() or c in "_./" for c in s):
        return s
    escaped = s.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def _ref(object_id: str, comment: str) -> str:
    return f"{object_id} /* {comment} */"


def _venv_activate_script() -> str:
    return f"{sys.prefix.replace(chr(92), '/')}/bin/activate"


class _PBXWriter:
    """Accumulates objects per-ISA so sections can be emitted in Xcode's usual order."""

    def __init__(self):
        self.sections: dict[str, list[str]] = {}

    def add(self, isa: str, object_id: str, comment: str, body: str):
        self.sections.setdefault(isa, []).append(
            f"\t\t{_ref(object_id, comment)} = {body};" if comment else f"\t\t{object_id} = {body};"
        )

    def render(self) -> str:
        parts = []
        for isa in sorted(self.sections):
            parts.append(f"/* Begin {isa} section */")
            parts.extend(self.sections[isa])
            parts.append(f"/* End {isa} section */")
        return "\n".join(parts)


def _build_body(fields: list[tuple[str, str]]) -> str:
    inner = "\n".join(f"\t\t\t{k} = {v};" for k, v in fields)
    return "{\n" + inner + "\n\t\t}"


def _list_field(items: list[str]) -> str:
    if not items:
        return "(\n\t\t\t)"
    inner = "\n".join(f"\t\t\t\t{item}," for item in items)
    return "(\n" + inner + "\n\t\t\t)"


def generate_xcode(engine_path: str, project_path: str, modules, args):
    project_name = (project_path.rstrip("/").rsplit("/", 1)[-1]) or "Artifact"
    xcodeproj_dir = f"{project_path}/{project_name}.xcodeproj"
    activate_script = _venv_activate_script()

    writer = _PBXWriter()

    module_file_refs: dict[str, list[str]] = {}
    for module in modules:
        refs = []
        for path in module.source_files + module.header_files:
            ext = os.path.splitext(path)[1]
            file_type = _FILE_TYPES.get(ext)
            if not file_type:
                continue
            file_id = _oid(f"fileref:{path}")
            name = os.path.basename(path)
            writer.add("PBXFileReference", file_id, name, _build_body([
                ("isa", "PBXFileReference"),
                ("lastKnownFileType", file_type),
                ("name", _pbx_str(name)),
                ("path", _pbx_str(path)),
                ("sourceTree", '"<absolute>"'),
            ]))
            refs.append(_ref(file_id, name))
        module_file_refs[module.name] = refs

    engine_modules = [m for m in modules if m.category == "Engine"]
    project_modules = [m for m in modules if m.category == "Project"]

    def module_group_id(module_name):
        return _oid(f"group:module:{module_name}")

    for module in modules:
        group_id = module_group_id(module.name)
        writer.add("PBXGroup", group_id, module.name, _build_body([
            ("isa", "PBXGroup"),
            ("children", _list_field(module_file_refs[module.name])),
            ("name", _pbx_str(module.name)),
            ("sourceTree", '"<group>"'),
        ]))

    engine_group_id = _oid("group:Engine")
    project_group_id = _oid("group:Project")
    products_group_id = _oid("group:Products")

    writer.add("PBXGroup", engine_group_id, "Engine", _build_body([
        ("isa", "PBXGroup"),
        ("children", _list_field([_ref(module_group_id(m.name), m.name) for m in engine_modules])),
        ("name", "Engine"),
        ("sourceTree", '"<group>"'),
    ]))
    writer.add("PBXGroup", project_group_id, "Project", _build_body([
        ("isa", "PBXGroup"),
        ("children", _list_field([_ref(module_group_id(m.name), m.name) for m in project_modules])),
        ("name", "Project"),
        ("sourceTree", '"<group>"'),
    ]))
    writer.add("PBXGroup", products_group_id, "Products", _build_body([
        ("isa", "PBXGroup"),
        ("children", _list_field([])),
        ("name", "Products"),
        ("sourceTree", '"<group>"'),
    ]))

    main_group_id = _oid("group:main")
    main_children = [_ref(engine_group_id, "Engine")]
    if project_modules:
        main_children.append(_ref(project_group_id, "Project"))
    main_children.append(_ref(products_group_id, "Products"))
    writer.add("PBXGroup", main_group_id, None, _build_body([
        ("isa", "PBXGroup"),
        ("children", _list_field(main_children)),
        ("sourceTree", '"<group>"'),
    ]))

    build_command = f"source '{activate_script}' && artifact build --configuration $CONFIGURATION --target MacOS"

    # Legacy targets don't compile through Xcode, but its indexer still reads HEADER_SEARCH_PATHS.
    header_search_paths = []
    seen_dirs = set()
    for module in modules:
        for d in module.include_dirs:
            if d not in seen_dirs:
                seen_dirs.add(d)
                header_search_paths.append(d)

    target_id = _oid("target:Artifact")
    target_config_ids = {c: _oid(f"config:target:{c}") for c in _CONFIGURATIONS}
    for c in _CONFIGURATIONS:
        writer.add("XCBuildConfiguration", target_config_ids[c], c, _build_body([
            ("isa", "XCBuildConfiguration"),
            ("buildSettings", _build_body([
                ("PRODUCT_NAME", "Artifact"),
                ("HEADER_SEARCH_PATHS", _list_field([_pbx_str(d) for d in header_search_paths])),
            ])),
            ("name", c),
        ]))
    target_config_list_id = _oid("configlist:target")
    writer.add("XCConfigurationList", target_config_list_id, None, _build_body([
        ("isa", "XCConfigurationList"),
        ("buildConfigurations", _list_field([_ref(target_config_ids[c], c) for c in _CONFIGURATIONS])),
        ("defaultConfigurationIsVisible", "0"),
        ("defaultConfigurationName", "Dev"),
    ]))

    writer.add("PBXLegacyTarget", target_id, "Artifact", _build_body([
        ("isa", "PBXLegacyTarget"),
        ("buildArgumentsString", _pbx_str(f'-lc "{build_command}"')),
        ("buildConfigurationList", _ref(target_config_list_id, 'Build configuration list for PBXLegacyTarget "Artifact"')),
        ("buildPhases", _list_field([])),
        ("buildToolPath", "/bin/bash"),
        ("buildWorkingDirectory", _pbx_str(project_path)),
        ("dependencies", _list_field([])),
        ("name", "Artifact"),
        ("passBuildSettingsInEnvironment", "1"),
        ("productName", "Artifact"),
    ]))

    project_config_ids = {c: _oid(f"config:project:{c}") for c in _CONFIGURATIONS}
    for c in _CONFIGURATIONS:
        writer.add("XCBuildConfiguration", project_config_ids[c], c, _build_body([
            ("isa", "XCBuildConfiguration"),
            ("buildSettings", _build_body([])),
            ("name", c),
        ]))
    project_config_list_id = _oid("configlist:project")
    writer.add("XCConfigurationList", project_config_list_id, None, _build_body([
        ("isa", "XCConfigurationList"),
        ("buildConfigurations", _list_field([_ref(project_config_ids[c], c) for c in _CONFIGURATIONS])),
        ("defaultConfigurationIsVisible", "0"),
        ("defaultConfigurationName", "Dev"),
    ]))

    project_id = _oid("project")
    writer.add("PBXProject", project_id, "Project object", _build_body([
        ("isa", "PBXProject"),
        ("attributes", _build_body([("LastUpgradeCheck", "1500")])),
        ("buildConfigurationList", _ref(project_config_list_id, f'Build configuration list for PBXProject "{project_name}"')),
        ("compatibilityVersion", _pbx_str("Xcode 14.0")),
        ("developmentRegion", "en"),
        ("hasScannedForEncodings", "0"),
        ("knownRegions", _list_field(["en", "Base"])),
        ("mainGroup", main_group_id),
        ("productRefGroup", _ref(products_group_id, "Products")),
        ("projectDirPath", '""'),
        ("projectRoot", '""'),
        ("targets", _list_field([_ref(target_id, "Artifact")])),
    ]))

    pbxproj = f"""// !$*UTF8*$!
{{
\tarchiveVersion = 1;
\tclasses = {{
\t}};
\tobjectVersion = 56;
\tobjects = {{

{writer.render()}

\t}};
\trootObject = {_ref(project_id, "Project object")};
}}
"""

    with smart_open(f"{xcodeproj_dir}/project.pbxproj") as f:
        f.write(pbxproj)

    _write_workspace_data(xcodeproj_dir)
    _write_scheme(xcodeproj_dir, project_path, target_id)


def _write_workspace_data(xcodeproj_dir: str):
    content = """<?xml version="1.0" encoding="UTF-8"?>
<Workspace
   version = "1.0">
   <FileRef
      location = "self:">
   </FileRef>
</Workspace>
"""
    with smart_open(f"{xcodeproj_dir}/project.xcworkspace/contents.xcworkspacedata") as f:
        f.write(content)


def _write_scheme(xcodeproj_dir: str, project_path: str, target_id: str):
    """A shared scheme whose Run action launches the built binary directly.

    PathRunnable (rather than the usual BuildableProductRunnable) is what lets
    a legacy/external-build target run an arbitrary executable rather than an
    Xcode-built product.
    """
    blueprint_name = "Artifact"
    content = f"""<?xml version="1.0" encoding="UTF-8"?>
<Scheme
   LastUpgradeVersion = "1500"
   version = "1.7">
   <BuildAction
      parallelizeBuildables = "NO"
      buildImplicitDependencies = "NO">
      <BuildActionEntries>
         <BuildActionEntry
            buildForTesting = "YES"
            buildForRunning = "YES"
            buildForProfiling = "YES"
            buildForArchiving = "YES"
            buildForAnalyzing = "YES">
            <BuildableReference
               BuildableIdentifier = "primary"
               BlueprintIdentifier = "{target_id}"
               BuildableName = "{blueprint_name}"
               BlueprintName = "{blueprint_name}"
               ReferencedContainer = "container:{os.path.basename(xcodeproj_dir)}">
            </BuildableReference>
         </BuildActionEntry>
      </BuildActionEntries>
   </BuildAction>
   <LaunchAction
      buildConfiguration = "Dev"
      selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"
      selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"
      launchStyle = "0"
      useCustomWorkingDirectory = "YES"
      customWorkingDirectory = "{project_path}"
      ignoresPersistentStateOnLaunch = "NO"
      debugDocumentVersioning = "YES"
      allowLocationSimulation = "YES">
      <PathRunnable
         FilePath = "{project_path}/Binaries/Artifact">
      </PathRunnable>
   </LaunchAction>
</Scheme>
"""
    with smart_open(f"{xcodeproj_dir}/xcshareddata/xcschemes/{blueprint_name}.xcscheme") as f:
        f.write(content)
