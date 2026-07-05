import hashlib
import os
import sys
from SDK.Util import smart_open

# Matches BuildTool.Target.TargetType; $CONFIGURATION is the env var Xcode
# exports for the aggregate target's build script.
_CONFIGURATIONS = ["Debug", "Dev", "Dist"]

# Kept in sync with SDK.Platforms.get_cpp_platform_macro / BuildTool.Target so
# the indexer sees the same preprocessor state the real build compiles with.
_PLATFORM_DEFINES = ["AE_PLATFORM_MACOS"]
_CONFIG_DEFINES = {
    "Debug": "AE_TARGET_DEBUG",
    "Dev": "AE_TARGET_DEV",
    "Dist": "AE_TARGET_DIST",
}

_FILE_TYPES = {
    ".cpp": "sourcecode.cpp.cpp",
    ".cc": "sourcecode.cpp.cpp",
    ".cxx": "sourcecode.cpp.cpp",
    ".h": "sourcecode.c.h",
    ".hpp": "sourcecode.c.h",
    ".inl": "sourcecode.c.h",
    ".json": "text.json",
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


def _sh_single_quote(s: str) -> str:
    """Wrap a string in single quotes, safe for embedding in a shell script."""
    return "'" + s.replace("'", "'\\''") + "'"


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


def _add_file_ref(writer: _PBXWriter, path: str) -> str:
    ext = os.path.splitext(path)[1]
    file_type = _FILE_TYPES.get(ext, "text")
    file_id = _oid(f"fileref:{path}")
    name = os.path.basename(path)
    writer.add("PBXFileReference", file_id, name, _build_body([
        ("isa", "PBXFileReference"),
        ("lastKnownFileType", file_type),
        ("name", _pbx_str(name)),
        ("path", _pbx_str(path)),
        ("sourceTree", '"<absolute>"'),
    ]))
    return _ref(file_id, name)


def _build_group_tree(writer: _PBXWriter, module) -> list[str]:
    """A PBXGroup subtree mirroring the module's on-disk folders.

    Returns the child refs for the module's own group. Folders sort before files
    at each level, matching how Xcode presents a normal source tree.
    """
    root: dict = {"dirs": {}, "files": []}
    all_files = sorted(set(module.source_files + module.header_files + module.other_files))
    for path in all_files:
        rel = os.path.relpath(path, module.path).replace("\\", "/")
        parts = rel.split("/")
        node = root
        for part in parts[:-1]:
            node = node["dirs"].setdefault(part, {"dirs": {}, "files": []})
        node["files"].append(path)

    def emit(node: dict, folder_rel: str) -> list[str]:
        children = []
        for dname in sorted(node["dirs"]):
            child_rel = f"{folder_rel}/{dname}" if folder_rel else dname
            sub_children = emit(node["dirs"][dname], child_rel)
            group_id = _oid(f"group:{module.name}:{child_rel}")
            writer.add("PBXGroup", group_id, dname, _build_body([
                ("isa", "PBXGroup"),
                ("children", _list_field(sub_children)),
                ("name", _pbx_str(dname)),
                ("sourceTree", '"<group>"'),
            ]))
            children.append(_ref(group_id, dname))
        for path in sorted(node["files"]):
            children.append(_add_file_ref(writer, path))
        return children

    return emit(root, "")


def _third_party_include_dirs(modules) -> list[str]:
    """Include dirs for vendored third-party projects the indexer would otherwise miss.

    The real build gets these transitively from each module's AddAdditionalCMakeProjects
    (and find_package results); the indexer has no CMake, so approximate them:
    each vendored project's root (glm-style <root>/glm) and its include/ subdir
    (glfw/spdlog-style), plus the common prefixes where find_package lands system
    SDKs such as Vulkan.
    """
    dirs = []
    for module in modules:
        for rel in module.module.AddAdditionalCMakeProjects:
            proj = f"{module.path}/{rel}"
            for candidate in (proj, f"{proj}/include"):
                if os.path.isdir(candidate):
                    dirs.append(candidate)

    system_prefixes = ["/usr/local/include", "/opt/homebrew/include"]
    vulkan_sdk = os.environ.get("VULKAN_SDK")
    if vulkan_sdk:
        system_prefixes.append(f"{vulkan_sdk}/include")
    dirs += [p for p in system_prefixes if os.path.isdir(p)]

    seen, unique = set(), []
    for d in dirs:
        if d not in seen:
            seen.add(d)
            unique.append(d)
    return unique


def _index_build_settings(configuration: str, header_search_paths: list[str], product_name: str) -> list[tuple[str, str]]:
    """Settings that let Xcode's indexer parse the sources the way the real build does.

    SDKROOT lets clang find the platform/std headers; the standard, defines and
    header search paths make cross-file symbol resolution match the real build.
    """
    defines = _PLATFORM_DEFINES + [_CONFIG_DEFINES[configuration], "$(inherited)"]
    return [
        ("ALWAYS_SEARCH_USER_PATHS", "NO"),
        ("CLANG_CXX_LANGUAGE_STANDARD", '"c++20"'),
        ("CLANG_CXX_LIBRARY", '"libc++"'),
        ("GCC_PREPROCESSOR_DEFINITIONS", _list_field([_pbx_str(d) for d in defines])),
        ("HEADER_SEARCH_PATHS", _list_field([_pbx_str(d) for d in header_search_paths])),
        ("PRODUCT_NAME", _pbx_str(product_name)),
        ("SDKROOT", "macosx"),
        ("SUPPORTED_PLATFORMS", "macosx"),
    ]


def generate_xcode(engine_path: str, project_path: str, modules, args):
    project_name = (project_path.rstrip("/").rsplit("/", 1)[-1]) or "Artifact"
    xcodeproj_dir = f"{project_path}/{project_name}.xcodeproj"
    activate_script = _venv_activate_script()

    writer = _PBXWriter()

    engine_modules = [m for m in modules if m.category == "Engine"]
    project_modules = [m for m in modules if m.category == "Project"]

    def module_group_id(module_name):
        return _oid(f"group:module:{module_name}")

    for module in modules:
        children = _build_group_tree(writer, module)
        writer.add("PBXGroup", module_group_id(module.name), module.name, _build_body([
            ("isa", "PBXGroup"),
            ("children", _list_field(children)),
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
        ("children", _list_field([_ref(_oid("product:ArtifactIndex"), "libArtifactIndex.a")])),
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

    # The indexer reads HEADER_SEARCH_PATHS to resolve includes for every file.
    header_search_paths = []
    seen_dirs = set()
    for module in modules:
        for d in module.include_dirs:
            if d not in seen_dirs:
                seen_dirs.add(d)
                header_search_paths.append(d)
    for d in _third_party_include_dirs(modules):
        if d not in seen_dirs:
            seen_dirs.add(d)
            header_search_paths.append(d)

    
    generation_path = os.environ.get("PATH", "")
    script_phase_id = _oid("scriptphase:Artifact")
    shell_script = (
        f"source '{activate_script}'\n"
        f"export PATH={_sh_single_quote(generation_path)}:\"$PATH\"\n"
        'artifact build --configuration "$CONFIGURATION" --target MacOS --skip-ide-project\n'
    )
    writer.add("PBXShellScriptBuildPhase", script_phase_id, "Build Artifact", _build_body([
        ("isa", "PBXShellScriptBuildPhase"),
        ("alwaysOutOfDate", "1"),
        ("buildActionMask", "2147483647"),
        ("files", _list_field([])),
        ("inputPaths", _list_field([])),
        ("name", _pbx_str("Build Artifact")),
        ("outputPaths", _list_field([])),
        ("runOnlyForDeploymentPostprocessing", "0"),
        ("shellPath", "/bin/bash"),
        ("shellScript", _pbx_str(shell_script)),
        ("showEnvVarsInLog", "0"),
    ]))

    target_id = _oid("target:Artifact")
    target_config_ids = {c: _oid(f"config:target:{c}") for c in _CONFIGURATIONS}
    for c in _CONFIGURATIONS:
        writer.add("XCBuildConfiguration", target_config_ids[c], c, _build_body([
            ("isa", "XCBuildConfiguration"),
            ("buildSettings", _build_body(_index_build_settings(c, header_search_paths, "Artifact"))),
            ("name", c),
        ]))
    target_config_list_id = _oid("configlist:target")
    writer.add("XCConfigurationList", target_config_list_id, None, _build_body([
        ("isa", "XCConfigurationList"),
        ("buildConfigurations", _list_field([_ref(target_config_ids[c], c) for c in _CONFIGURATIONS])),
        ("defaultConfigurationIsVisible", "0"),
        ("defaultConfigurationName", "Dev"),
    ]))

    writer.add("PBXAggregateTarget", target_id, "Artifact", _build_body([
        ("isa", "PBXAggregateTarget"),
        ("buildConfigurationList", _ref(target_config_list_id, 'Build configuration list for PBXAggregateTarget "Artifact"')),
        ("buildPhases", _list_field([_ref(script_phase_id, "Build Artifact")])),
        ("dependencies", _list_field([])),
        ("name", "Artifact"),
        ("productName", "Artifact"),
    ]))

    # A native static-library target that exists purely so Xcode's indexer has
    # somewhere to hang the sources. Xcode only indexes files that are members of
    # a target with a compile-sources phase, so the aggregate target above (which
    # owns no files) yields no intellisense on its own. This target is never in
    # the scheme's build action, so `artifact build` remains the only real build;
    # Xcode still indexes it in the background, giving cross-file symbols.
    index_target_id = _oid("target:ArtifactIndex")
    index_config_ids = {c: _oid(f"config:index:{c}") for c in _CONFIGURATIONS}
    for c in _CONFIGURATIONS:
        writer.add("XCBuildConfiguration", index_config_ids[c], c, _build_body([
            ("isa", "XCBuildConfiguration"),
            ("buildSettings", _build_body(_index_build_settings(c, header_search_paths, "ArtifactIndex"))),
            ("name", c),
        ]))
    index_config_list_id = _oid("configlist:index")
    writer.add("XCConfigurationList", index_config_list_id, None, _build_body([
        ("isa", "XCConfigurationList"),
        ("buildConfigurations", _list_field([_ref(index_config_ids[c], c) for c in _CONFIGURATIONS])),
        ("defaultConfigurationIsVisible", "0"),
        ("defaultConfigurationName", "Dev"),
    ]))

    source_build_files = []
    for module in modules:
        for path in module.source_files:
            build_file_id = _oid(f"buildfile:{path}")
            name = os.path.basename(path)
            writer.add("PBXBuildFile", build_file_id, f"{name} in Sources", _build_body([
                ("isa", "PBXBuildFile"),
                ("fileRef", _ref(_oid(f"fileref:{path}"), name)),
            ]))
            source_build_files.append(_ref(build_file_id, f"{name} in Sources"))

    index_sources_phase_id = _oid("sourcesphase:ArtifactIndex")
    writer.add("PBXSourcesBuildPhase", index_sources_phase_id, "Sources", _build_body([
        ("isa", "PBXSourcesBuildPhase"),
        ("buildActionMask", "2147483647"),
        ("files", _list_field(source_build_files)),
        ("runOnlyForDeploymentPostprocessing", "0"),
    ]))

    index_product_id = _oid("product:ArtifactIndex")
    writer.add("PBXFileReference", index_product_id, "libArtifactIndex.a", _build_body([
        ("isa", "PBXFileReference"),
        ("explicitFileType", '"archive.ar"'),
        ("includeInIndex", "0"),
        ("path", "libArtifactIndex.a"),
        ("sourceTree", "BUILT_PRODUCTS_DIR"),
    ]))

    writer.add("PBXNativeTarget", index_target_id, "ArtifactIndex", _build_body([
        ("isa", "PBXNativeTarget"),
        ("buildConfigurationList", _ref(index_config_list_id, 'Build configuration list for PBXNativeTarget "ArtifactIndex"')),
        ("buildPhases", _list_field([_ref(index_sources_phase_id, "Sources")])),
        ("buildRules", _list_field([])),
        ("dependencies", _list_field([])),
        ("name", "ArtifactIndex"),
        ("productName", "ArtifactIndex"),
        ("productReference", _ref(index_product_id, "libArtifactIndex.a")),
        ("productType", '"com.apple.product-type.library.static"'),
    ]))

    project_config_ids = {c: _oid(f"config:project:{c}") for c in _CONFIGURATIONS}
    for c in _CONFIGURATIONS:
        writer.add("XCBuildConfiguration", project_config_ids[c], c, _build_body([
            ("isa", "XCBuildConfiguration"),
            ("buildSettings", _build_body(_index_build_settings(c, header_search_paths, "Artifact"))),
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
        ("targets", _list_field([_ref(target_id, "Artifact"), _ref(index_target_id, "ArtifactIndex")])),
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
    an aggregate target run an arbitrary executable rather than an Xcode-built
    product.
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
