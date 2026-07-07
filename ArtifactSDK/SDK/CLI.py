import argparse

from colorama import Fore, Style

from HeaderTool.HeaderTool import HeaderTool
from BuildTool.Generate import generate_cmake
from BuildTool.Build import build_cmake
from BuildTool.IDEGen import generate_ide_project
from SDK.Paths import get_engine_path, get_project_path
from SDK.Platforms import get_current_platform
from SDK.Util import png_to_ico
from SDK.Job import Job, JobError
from Lint.Lint import lint_files
import os
import sys
import subprocess
import shutil

def _generate(args):
    engine_path = get_engine_path()
    project_path = get_project_path()

    # Clean build if requested
    clean = getattr(args, 'clean', False)
    if clean:
        build_dir = f"{project_path}/Build"
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir)

    with Job("Generating project files"):
        generate_cmake(project_path, args)

        # Generate reflection code for classes in Modules
        header_tool = HeaderTool()
        header_tool.collect_headers(f"{engine_path}/Modules", engine_path)
        header_tool.collect_headers(f"{project_path}/Modules", project_path)
        header_tool.generate()

        png_to_ico(f"{project_path}/Content/Icons/Icon.png", f"{project_path}/Build/Intermediate/Resources/IconWin64.ico")

    # IDE-triggered builds pass --skip-ide-project: regenerating the .xcodeproj /
    # .vcxproj while the IDE is mid-build rewrites the project file under it,
    # failing the build phase and invalidating the index.
    if not getattr(args, "skip_ide_project", False):
        with Job("Generating IDE project files"):
            generate_ide_project(engine_path, project_path, args)

    from SDK.Create import generate_sdk_activation_scripts
    generate_sdk_activation_scripts(project_path, engine_path)

def cmd_generate(args):
    try:
        _generate(args)
    except KeyboardInterrupt:
        print("Generation cancelled by user.")
        sys.exit(1)
    except JobError as e:
        sys.exit(e.returncode)

def cmd_build(args):
    try:
        _generate(args)

        with Job("Building", dump_on_error=False) as job:
            build_cmake(job)
    except KeyboardInterrupt:
        print("Build cancelled by user.")
        sys.exit(1)
    except JobError as e:
        sys.exit(e.returncode)

def cmd_run(args):
    args.target = get_current_platform().name
    args.configuration = "Dev"  # Always run the Dev configuration for better debugging experience
    cmd_build(args)  # Ensure the engine is built before running
    project_path = os.getcwd()
    try:
        subprocess.run([f"{project_path}/Binaries/Artifact"], check=True)
    except KeyboardInterrupt:
        pass  # Allow graceful exit on Ctrl+C
    except subprocess.CalledProcessError as e:
        print(f"Error occurred while running the engine: {e}")

def cmd_cook(args):
    args.target = get_current_platform().name
    args.configuration = "Dev"
    args.clean = args.clean if hasattr(args, "clean") else False
    cmd_build(args)  # Ensure the engine is built before running the AssetCooker
    project_path = os.getcwd()
    cook_dir = f"{project_path}/Build/Intermediate/Cooked"
    os.makedirs(cook_dir, exist_ok=True)
    try:
        with Job("Cooking") as job:
            job.run([
                f"{project_path}/Binaries/Artifact",
                "-EngineClass=AssetCookerEngine",
                f"-CookDirectory={cook_dir}",
            ], check=True,  # non-zero exit fails the job
               # AssetCooker logs "[current/total] Cooking asset: ..." (see AssetCooker.cpp).
               progress_pattern=r"\[(\d+)/(\d+)\]")
    except KeyboardInterrupt:
        pass  # Allow graceful exit on Ctrl+C
    except JobError as e:
        sys.exit(e.returncode)

def cmd_package(args):
    project_path = os.getcwd()
    cmd_cook(args)  # Ensure assets are cooked before packaging
    # copy cooked assets to Content directory so they get included in the package
    cooked_src = f"{project_path}/Build/Intermediate/Cooked"
    content_dest = f"{project_path}/Dist/Cooked"
    if os.path.exists(content_dest):
        shutil.rmtree(content_dest)
    shutil.copytree(cooked_src, content_dest)

    args.target = get_current_platform().name
    args.configuration = "Dist"  # Always package the Dist configuration
    args.packaged = True  # Ensure the AE_PACKAGED macro is defined for packaging
    args.clean = True # Clean build artifacts before packaging to ensure a clean package
    cmd_build(args)
    
    os.makedirs(f"{project_path}/Dist", exist_ok=True)
    
    if args.target == "MacOS":
        from Package.MacOS import package_for_macos
        with Job("Packaging"):
            package_for_macos(project_path)
    elif args.target == "Win64":
        from Package.Win64 import package_for_win64
        with Job("Packaging"):
            package_for_win64(project_path)
    else:
        print("Only MacOS and Win64 packaging are implemented so far")
        exit(1)

def cmd_lint(args):
    files = []
    for root, dirs, filenames in os.walk(get_engine_path()):
        for name in filenames:
            if name.endswith((".cpp", ".h")):
                files.append(os.path.join(root, name))
    for root, dirs, filenames in os.walk(get_project_path()):
        for name in filenames:
            if name.endswith((".cpp", ".h")):
                files.append(os.path.join(root, name))
    lint_errors = lint_files(files, fix=args.fix)
    if lint_errors > 0:
        print(f"{Fore.RED}Found {lint_errors} linting errors!{Style.RESET_ALL}")
    else:
        print(f"{Fore.GREEN}No linting errors found.{Style.RESET_ALL}")
    sys.exit(lint_errors)

def cmd_version(args):
    from SDK.Version import get_version_string
    print(f"Artifact SDK version {get_version_string()}")

def cmd_location(args):
    from SDK.Paths import get_engine_path
    print(get_engine_path())

def cmd_create_project(args):
    from SDK.Create import create_project
    create_project(args.name)

def cmd_create_module(args):
    from SDK.Create import create_module
    create_module(args.name)

def cmd_create_type(args):
    from SDK.Create import create_reflected_type
    create_reflected_type(args.kind, args.name, args.parent, args.module)

def main():
    parser = argparse.ArgumentParser(description="Artifact Engine Build Tool")
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate_args_parser = argparse.ArgumentParser(add_help=False)
    generate_args_parser.add_argument("--target", choices=["Win64", "MacOS", "Linux"], default=get_current_platform().name, help="Target platform to generate/build for")
    generate_args_parser.add_argument("--configuration", choices=["Debug", "Dev", "Dist"], default="Dev", help="Build configuration")
    generate_args_parser.add_argument("--packaged", action="store_true", default=False, help="Whether to build a packaged version (binary may be embedded into an application bundle)")
    generate_args_parser.add_argument("--clean", action="store_true", default=False, help="Clean build artifacts before generating/building")
    generate_args_parser.add_argument("--skip-ide-project", action="store_true", default=False, help="Skip regenerating the native IDE project (used by IDE-triggered builds so the project file isn't rewritten mid-build)")

    generate_parser = subparsers.add_parser("generate", parents=[generate_args_parser], help="Generate project files (CMake + native IDE project) without building")
    generate_parser.set_defaults(func=cmd_generate)

    build_parser = subparsers.add_parser("build", parents=[generate_args_parser], help="Build the engine")
    build_parser.set_defaults(func=cmd_build)

    run_parser = subparsers.add_parser("run", help="Run the engine")
    run_parser.add_argument("--clean", action="store_true", default=False, help="Clean build artifacts before running")
    run_parser.set_defaults(func=cmd_run)

    cook_parser = subparsers.add_parser("cook", help="Cook assets")
    cook_parser.set_defaults(func=cmd_cook)

    package_parser = subparsers.add_parser("package", help="Package project")
    package_parser.set_defaults(func=cmd_package)

    lint_parser = subparsers.add_parser("lint", help="Lint C++/Header files")
    lint_parser.add_argument("--fix", action="store_true", default=False, help="Automatically fix fixable lint errors in place")
    lint_parser.set_defaults(func=cmd_lint)

    version_parser = subparsers.add_parser("version", help="Show engine version")
    version_parser.set_defaults(func=cmd_version)

    location_parser = subparsers.add_parser("location", help="Print the engine path to the terminal")
    location_parser.set_defaults(func=cmd_location)

    create_parser = subparsers.add_parser("create", help="Scaffold a project, module, or reflected type")
    create_subparsers = create_parser.add_subparsers(dest="create_command", required=True)

    create_project_parser = create_subparsers.add_parser("project", help="Create a new project in a subdirectory")
    create_project_parser.add_argument("name", help="Project name (also the subdirectory it is created in)")
    create_project_parser.set_defaults(func=cmd_create_project)

    create_module_parser = create_subparsers.add_parser("module", help="Create a new module in the current engine/project")
    create_module_parser.add_argument("name", help="Module name")
    create_module_parser.set_defaults(func=cmd_create_module)

    for kind in ("class", "struct", "node", "component"):
        type_parser = create_subparsers.add_parser(kind, help=f"Create a new reflected {kind}")
        type_parser.add_argument("name", help=f"{kind.capitalize()} name")
        if kind != "struct":
            type_parser.add_argument("parent", nargs="?", default=None, help="Parent class (defaults per kind)")
        type_parser.add_argument("--module", default=None, help="Module to create the type in (required if ambiguous)")
        type_parser.set_defaults(func=cmd_create_type, kind=kind, parent=None)

    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
