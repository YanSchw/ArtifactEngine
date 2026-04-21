import argparse

from HeaderTool.HeaderTool import HeaderTool
from BuildTool.Generate import generate_cmake
from BuildTool.Build import build_cmake
from BuildTool.Paths import get_engine_path
from BuildTool.Platforms import get_current_platform
from BuildTool.Util import png_to_ico
import os
import subprocess

def cmd_build(args):
    print("Building the engine...")
    engine_path = get_engine_path()
    project_path = os.getcwd().replace("\\", "/")
    generate_cmake(project_path, args)  # Generate CMakeLists.txt in the project directory

    # Generate reflection code for classes in Modules
    header_tool = HeaderTool()
    header_tool.collect_headers(f"{engine_path}/Modules")
    header_tool.collect_headers(f"{project_path}/Modules")
    header_tool.generate()
    png_to_ico(f"{project_path}/Content/Icons/Icon.png", f"{project_path}/Build/Intermediate/Resources/IconWin64.ico")
    build_cmake()

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
    print("Cooking assets not yet implemented")

def cmd_package(args):
    args.target = get_current_platform().name
    args.configuration = "Dist"  # Always package the Dist configuration
    args.packaged = True  # Ensure the AE_PACKAGED macro is defined for packaging
    cmd_build(args)
    
    project_path = os.getcwd()
    os.makedirs(f"{project_path}/Dist", exist_ok=True)
    
    if args.target == "MacOS":
        from Package.MacOS import package_for_macos
        package_for_macos(project_path)
    else:
        print("Only MacOS packaging is implemented so far")
        exit(1)

def cmd_version(args):
    from BuildTool.Version import get_version_string
    print(f"Artifact SDK version {get_version_string()}")

def main():
    parser = argparse.ArgumentParser(description="Artifact Engine Build Tool")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="Build the engine")
    build_parser.add_argument("--target", choices=["Win64", "MacOS", "Linux"], default=get_current_platform().name, help="Target platform to build for")
    build_parser.add_argument("--configuration", choices=["Debug", "Dev", "Dist"], default="Dev", help="Build configuration")
    build_parser.add_argument("--packaged", action="store_true", default=False, help="Whether to build a packaged version (binary may be embedded into an application bundle)")
    build_parser.set_defaults(func=cmd_build)

    run_parser = subparsers.add_parser("run", help="Run the engine")
    run_parser.set_defaults(func=cmd_run)

    cook_parser = subparsers.add_parser("cook", help="Cook assets")
    cook_parser.set_defaults(func=cmd_cook)

    package_parser = subparsers.add_parser("package", help="Package project")
    package_parser.set_defaults(func=cmd_package)

    version_parser = subparsers.add_parser("version", help="Show engine version")
    version_parser.set_defaults(func=cmd_version)

    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
