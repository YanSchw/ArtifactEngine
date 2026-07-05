from SDK.Platforms import PlatformType, get_current_platform
from BuildTool.IDEGen.Modules import collect_ide_modules


def generate_ide_project(engine_path: str, project_path: str, args):
    """Generate the native IDE project for the current platform.

    Windows gets a Visual Studio solution, macOS an Xcode project.
    """
    platform = get_current_platform()
    target = platform.name

    modules = collect_ide_modules(engine_path, project_path, target)

    if platform == PlatformType.Win64:
        from BuildTool.IDEGen.VisualStudio import generate_visual_studio
        generate_visual_studio(engine_path, project_path, modules, args)
    elif platform == PlatformType.MacOS:
        from BuildTool.IDEGen.Xcode import generate_xcode
        generate_xcode(engine_path, project_path, modules, args)
    # Linux has no IDE project generator (yet); CMake alone still works there.
