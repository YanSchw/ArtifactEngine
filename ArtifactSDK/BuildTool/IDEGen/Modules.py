import os
from BuildTool.Module import ArtifactModule

# Vendored code (e.g. GLM_Math's ThirdParty, hundreds of headers) is excluded
# from IDE generation to keep the generated solution/project usable.
_EXCLUDED_DIR_NAMES = {"ThirdParty", "Build", "Intermediate", "Binaries", "Dist"}

_SOURCE_EXTENSIONS = (".cpp", ".cc", ".cxx")
_HEADER_EXTENSIONS = (".h", ".hpp", ".inl")
_IGNORED_FILE_SUFFIXES = (".vcxproj", ".vcxproj.filters", ".vcxproj.user", ".DS_Store")


class IDEModule:
    """A module plus everything an IDE project generator needs about it."""

    def __init__(self, name: str, path: str, category: str, module: ArtifactModule):
        self.name = name
        self.path = path  # absolute, forward slashes
        self.category = category  # "Engine" or "Project"
        self.module = module
        self.source_files: list[str] = []
        self.header_files: list[str] = []
        self.other_files: list[str] = []  # everything else (Module.json, CMakeLists.txt, shaders, ...)
        self.include_dirs: list[str] = []  # transitive, absolute paths
        self.guid = None  # filled in by the Visual Studio generator

    def __repr__(self):
        return f"IDEModule(name={self.name}, category={self.category})"


def _list_module_names(modules_dir: str) -> list[str]:
    if not os.path.isdir(modules_dir):
        return []
    return sorted(
        name for name in os.listdir(modules_dir)
        if os.path.isdir(f"{modules_dir}/{name}") and os.path.exists(f"{modules_dir}/{name}/Module.json")
    )


def _collect_module_files(module_dir: str):
    """Every file under a module, split into sources/headers/other."""
    sources, headers, other = [], [], []
    for dirpath, dirnames, filenames in os.walk(module_dir):
        dirnames[:] = [d for d in dirnames if d not in _EXCLUDED_DIR_NAMES]
        for filename in filenames:
            if filename.endswith(_IGNORED_FILE_SUFFIXES):
                continue
            full = f"{dirpath}/{filename}".replace("\\", "/")
            if filename.endswith(_SOURCE_EXTENSIONS):
                sources.append(full)
            elif filename.endswith(_HEADER_EXTENSIONS):
                headers.append(full)
            else:
                other.append(full)
    return sorted(sources), sorted(headers), sorted(other)


def _expand_include_dirs(module: IDEModule, modules_by_name: dict[str, IDEModule]) -> list[str]:
    """Own include dirs plus every (transitively) imported module's exported ones."""
    include_dirs = [module.path]
    include_dirs += [f"{module.path}/{p}" for p in module.module.IncludePaths]

    seen = set()
    to_visit = list(module.module.ImportModules)
    while to_visit:
        dep_name = to_visit.pop()
        if dep_name in seen:
            continue
        seen.add(dep_name)
        dep = modules_by_name.get(dep_name)
        if dep is None:
            continue
        include_dirs += [f"{dep.path}/{p}" for p in dep.module.ExportIncludePaths]
        include_dirs += [f"{dep.path}/{p}" for p in dep.module.IncludePaths]
        to_visit.extend(dep.module.ImportModules)

    unique = []
    seen_dirs = set()
    for d in include_dirs:
        if d not in seen_dirs:
            seen_dirs.add(d)
            unique.append(d)
    return unique


def collect_ide_modules(engine_path: str, project_path: str, target_platform: str) -> list[IDEModule]:
    """Discover Engine and Project modules for IDE project generation.

    Modules under `engine_path/Modules` are grouped as "Engine"; modules under
    `project_path/Modules` are "Project" — unless the two paths are the same
    directory, in which case there's nothing to split and everything found there is "Engine".
    """
    engine_path = engine_path.replace("\\", "/").rstrip("/")
    project_path = project_path.replace("\\", "/").rstrip("/")
    same_repo = os.path.normcase(os.path.normpath(engine_path)) == os.path.normcase(os.path.normpath(project_path))

    modules: list[IDEModule] = []
    seen_names = set()

    def add(modules_dir: str, category: str):
        for name in _list_module_names(modules_dir):
            if name in seen_names:
                continue
            module_dir = f"{modules_dir}/{name}"
            artifact_module = ArtifactModule.load_from_json(module_dir)
            if not artifact_module.supports_platform(target_platform):
                continue
            seen_names.add(name)
            modules.append(IDEModule(name=name, path=module_dir, category=category, module=artifact_module))

    add(f"{engine_path}/Modules", "Engine")
    if not same_repo:
        add(f"{project_path}/Modules", "Project")

    # Matches the include dir BuildTool.Generate.generate_cmake adds for every
    # module where HeaderTool writes the *.gen.h reflection headers.
    classes_dir = f"{project_path}/Build/Intermediate/Classes"

    modules_by_name = {m.name: m for m in modules}
    for module in modules:
        module.source_files, module.header_files, module.other_files = _collect_module_files(module.path)
        module.include_dirs = _expand_include_dirs(module, modules_by_name) + [classes_dir]

    return modules
