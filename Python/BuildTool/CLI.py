import argparse

from BuildTool.Generate import generate_cmake
from BuildTool.Build import build_cmake

def main():
    parser = argparse.ArgumentParser(description="Artifact Engine Build Tool")
    parser.add_argument("--build", action="store_true", help="Build the engine")
    # Add your CLI args here
    args = parser.parse_args()

    if args.build:
        print("Building the engine...")
        generate_cmake(".")  # Generate CMakeLists.txt in the current directory
        build_cmake()

if __name__ == "__main__":
    main()
