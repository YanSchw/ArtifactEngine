import os
import shutil
import subprocess
import sys
from colorama import Fore, Style
from BuildTool.Platforms import *
import re

MSVC_PATTERN = re.compile(
    r'^(.*)\((\d+)(?:,(\d+))?\):\s+(error|warning)\s+([A-Z0-9]+):\s+(.*)$'
)

GCC_CLANG_PATTERN = re.compile(
    r'^(.*?):(\d+)(?::(\d+))?:\s+(error|warning):\s+(.*)$'
)

def parse_output(output: str):
    errors = []
    warnings = []

    for line in output.splitlines():
        line = line.strip()

        msvc = MSVC_PATTERN.match(line)
        if msvc:
            file, line_no, col, level, code, msg = msvc.groups()
            entry = {
                "compiler": "msvc",
                "file": file,
                "line": int(line_no),
                "col": int(col) if col else None,
                "level": level,
                "code": code,
                "message": msg.strip()
            }
            (errors if level == "error" else warnings).append(entry)
            continue

        gcc = GCC_CLANG_PATTERN.match(line)
        if gcc:
            file, line_no, col, level, msg = gcc.groups()
            entry = {
                "compiler": "gcc/clang",
                "file": file,
                "line": int(line_no),
                "col": int(col) if col else None,
                "level": level,
                "message": msg.strip()
            }
            (errors if level == "error" else warnings).append(entry)
            continue

    return errors, warnings

def current_platform_generator():
    platform = get_current_platform()
    if platform == PlatformType.Win64:
        return "Visual Studio 17 2022"
    elif platform == PlatformType.MacOS:
        return "Xcode"
    elif platform == PlatformType.Linux:
        return "Unix Makefiles"
    else:
        raise RuntimeError(f"Unsupported development platform: {platform}")

def build_cmake(clean: bool = False):
    if clean:
        if os.path.exists("Build"):
            shutil.rmtree("Build")

    if os.path.exists("Binaries"):
        shutil.rmtree("Binaries")

    subprocess.run(["cmake", "-S", ".", "-G", current_platform_generator(), "-B", "Build"], check=True)
    proc = subprocess.run(
        ["cmake", "--build", "Build", "--config", "Debug"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    combined = proc.stdout + "\n" + proc.stderr
    with open("Build/CMakeBuild.log", "w") as log_file:
        log_file.write(combined)

    errors, warnings = parse_output(combined)

    if proc.returncode == 0:
        if len(warnings) > 0:
            for warning in warnings:
                print(f"{Fore.YELLOW}Warning: {warning['file']}:{warning['line']}:{warning['col']} - {warning['message']} ({warning.get('code', '')}){Style.RESET_ALL}")
            print(f"{Fore.GREEN}** Build succeeded with {len(warnings)} warnings. **{Style.RESET_ALL} See {os.path.abspath('Build/CMakeBuild.log')} for details.")
        else:
            print(f"{Fore.GREEN}** Build succeeded with no warnings. **{Style.RESET_ALL}")
    else:
        for warning in warnings:
            print(f"{Fore.YELLOW}Warning: {warning['file']}:{warning['line']}:{warning['col']} - {warning['message']} ({warning.get('code', '')}){Style.RESET_ALL}")
        for error in errors:
            print(f"{Fore.RED}Error: {error['file']}:{error['line']}:{error['col']} - {error['message']} ({error.get('code', '')}){Style.RESET_ALL}")
        print(f"{Fore.RED}** Build failed with {len(errors)} errors and {len(warnings)} warnings. **{Style.RESET_ALL} See {os.path.abspath('Build/CMakeBuild.log')} for details.")
        sys.exit(proc.returncode)