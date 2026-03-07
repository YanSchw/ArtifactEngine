import argparse

from BuildTool.Generate import generate_cmake
from BuildTool.Reflection import generate_class_cpp
from BuildTool.Build import build_cmake
from BuildTool.Paths import get_engine_path
from BuildTool.Platforms import get_current_platform
import os
import subprocess

def cmd_build(args):
    print("Building the engine...")
    engine_path = get_engine_path()
    project_path = os.getcwd().replace("\\", "/")
    target_platform = args.target
    generate_cmake(project_path, target_platform)  # Generate CMakeLists.txt in the project directory

    # Generate reflection code for classes in Modules
    generate_class_cpp(f"{engine_path}/Modules")
    generate_class_cpp(f"{project_path}/Modules")
    build_cmake()

def cmd_run(args):
    args.target = get_current_platform().name
    cmd_build(args)  # Ensure the engine is built before running
    project_path = os.getcwd()
    subprocess.run([f"{project_path}/Binaries/Artifact"], check=True)

def cmd_cook(args):
    print("Cooking assets not yet implemented")

def cmd_package(args):
    print("Packaging project not yet implemented")

def cmd_version(args):
    from BuildTool.Version import get_version_string
    print(f"Artifact Engine version {get_version_string()}")

def main():
    parser = argparse.ArgumentParser(description="Artifact Engine Build Tool")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="Build the engine")
    build_parser.add_argument("--target", choices=["Win64", "MacOS", "Linux"], default=get_current_platform().name, help="Target platform to build for")
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
