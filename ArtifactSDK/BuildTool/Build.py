import os
import shutil
import subprocess
from colorama import Fore, Style
from SDK.Platforms import *
from SDK.Job import JobError
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

# A single multi-config generator across every platform. "Multi-Config" keeps
# the config-specific CMAKE_RUNTIME_OUTPUT_DIRECTORY_<CONFIG> variables (used in
# the generated CMakeLists) working, so the binary still lands in Binaries/.
CMAKE_GENERATOR = "Ninja Multi-Config"

# Ninja prints "[finished/total] description" for every completed build edge.
NINJA_PROGRESS_PATTERN = re.compile(r'^\s*\[(\d+)/(\d+)\]\s*(.*)$')


class BuildError(JobError):
    """Raised when the CMake build fails; carries the process return code."""

    def __init__(self, returncode: int):
        super().__init__(f"Build failed with return code {returncode}", returncode)


def _find_vcvarsall():
    """Locate vcvarsall.bat for the latest Visual Studio with the VC toolset."""
    program_files = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = os.path.join(program_files, "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.exists(vswhere):
        return None
    result = subprocess.run(
        [vswhere, "-latest", "-products", "*",
         "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
         "-property", "installationPath"],
        capture_output=True, text=True,
    )
    install_path = result.stdout.strip().splitlines()
    if not install_path:
        return None
    vcvars = os.path.join(install_path[0], "VC", "Auxiliary", "Build", "vcvarsall.bat")
    return vcvars if os.path.exists(vcvars) else None


def build_environment():
    """Return the environment used for CMake/Ninja invocations.

    On macOS and Linux this is just the current environment (Ninja finds clang/
    gcc on PATH). On Windows the Ninja generator needs the MSVC toolchain on
    PATH; if cl.exe isn't already available we source vcvarsall.bat and merge in
    the resulting environment so `artifact build` works from a plain shell.
    """
    env = os.environ.copy()
    if get_current_platform() != PlatformType.Win64:
        return env
    if shutil.which("cl"):
        return env  # already inside a Developer Command Prompt
    vcvars = _find_vcvarsall()
    if not vcvars:
        return env  # fall back and let CMake try whatever compiler it finds
    result = subprocess.run(f'"{vcvars}" x64 && set', capture_output=True, text=True, shell=True)
    for line in result.stdout.splitlines():
        key, sep, value = line.partition("=")
        if sep:
            env[key] = value
    return env


def build_cmake(job=None):
    """Configure and build via CMake + Ninja.

    When a ``job`` is supplied, subprocess output is streamed into the job's
    live tail and ninja's [N/M] progress drives the spinner's progress bar and
    ETA. On success a warning summary (if any) is shown; on failure the parsed
    errors/warnings are reported and ``BuildError`` is raised.
    """
    if os.path.exists("Binaries"):
        shutil.rmtree("Binaries")

    env = build_environment()
    _clear_stale_cache()

    configure_cmd = ["cmake", "-S", ".", "-G", CMAKE_GENERATOR, "-B", "Build"]
    build_cmd = ["cmake", "--build", "Build", "--config", "Debug"]

    if job is not None:
        rc, configure_out = _run_streamed(configure_cmd, env, job)
        if rc != 0:
            _report_configure_failure(job, configure_out)
            raise BuildError(rc)
        returncode, combined = _run_streamed(build_cmd, env, job, parse_progress=True)
    else:
        subprocess.run(configure_cmd, check=True, env=env)
        proc = subprocess.run(build_cmd, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, env=env)
        returncode, combined = proc.returncode, proc.stdout

    with open("Build/CMakeBuild.log", "w") as log_file:
        log_file.write(combined)

    errors, warnings = parse_output(combined)
    _report_diagnostics(job, errors, warnings, returncode, combined)

    if returncode != 0:
        # Surface the failure through the job's error handling so the spinner
        # region is torn down before the diagnostics are shown.
        raise BuildError(returncode)


def _clear_stale_cache():
    """Wipe the CMake cache if it was configured with a different generator.

    Switching generators (e.g. the old Xcode/VS setup -> Ninja) leaves a
    CMakeCache.txt that CMake refuses to reuse. Only the cache and CMakeFiles
    are removed; the generated Intermediate sources are kept.
    """
    cache = os.path.join("Build", "CMakeCache.txt")
    if not os.path.exists(cache):
        return
    previous_generator = None
    with open(cache) as f:
        for line in f:
            if line.startswith("CMAKE_GENERATOR:"):
                previous_generator = line.split("=", 1)[1].strip()
                break
    if previous_generator and previous_generator != CMAKE_GENERATOR:
        os.remove(cache)
        cmake_files = os.path.join("Build", "CMakeFiles")
        if os.path.isdir(cmake_files):
            shutil.rmtree(cmake_files)


def _run_streamed(command, env, job, parse_progress: bool = False):
    """Run a subprocess, streaming output into the job and capturing it.

    When ``parse_progress`` is set, ninja's [N/M] lines drive the job's
    progress bar. Returns ``(returncode, combined_output)``.
    """
    proc = subprocess.Popen(command, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, bufsize=1, env=env)
    captured = []
    for line in proc.stdout:
        captured.append(line)
        match = NINJA_PROGRESS_PATTERN.match(line) if parse_progress else None
        if match:
            current, total, description = match.groups()
            job.set_progress(int(current), int(total))
            job.log(description)
        else:
            job.log(line)
    proc.wait()
    return proc.returncode, "".join(captured)


def _report_configure_failure(job, output):
    """Persist the tail of a failed CMake configure step."""
    tail = [line for line in output.splitlines() if line.strip()][-12:]
    for line in tail:
        _persist(job, f"{Style.DIM}{line}{Style.RESET_ALL}")
    _persist(job, f"{Fore.RED}✖ CMake configuration failed.{Style.RESET_ALL}")


def _format_diagnostic(level: str, diag: dict, project_path: str) -> str:
    """Format one compiler diagnostic as a coloured, project-relative line."""
    file = diag["file"]
    try:
        file = os.path.relpath(file, project_path)
    except ValueError:
        pass  # different drive on Windows; keep the absolute path
    location = f"{file}:{diag['line']}"
    if diag.get("col"):
        location += f":{diag['col']}"
    code = f" {Style.DIM}[{diag['code']}]{Style.RESET_ALL}" if diag.get("code") else ""
    colour = Fore.RED if level == "error" else Fore.YELLOW
    return (f"{colour}{level}{Style.RESET_ALL} "
            f"{Style.BRIGHT}{location}{Style.RESET_ALL}  {diag['message']}{code}")


def _dedupe(diagnostics: list[dict]) -> list[dict]:
    """Drop duplicate diagnostics (the same warning across many translation units)."""
    seen = set()
    unique = []
    for diag in diagnostics:
        key = (diag.get("file"), diag.get("line"), diag.get("col"), diag.get("message"))
        if key not in seen:
            seen.add(key)
            unique.append(diag)
    return unique


def _report_diagnostics(job, errors, warnings, returncode, combined):
    """Persist a readable warning/error report past the job's live region."""
    project_path = os.getcwd()
    log_path = os.path.abspath("Build/CMakeBuild.log")

    errors = _dedupe(errors)
    warnings = _dedupe(warnings)

    for warning in warnings:
        _persist(job, _format_diagnostic("warning", warning, project_path))
    for error in errors:
        _persist(job, _format_diagnostic("error", error, project_path))

    if returncode == 0:
        if warnings:
            _persist(job, f"{Fore.GREEN}✔ Build succeeded{Style.RESET_ALL} "
                          f"{Fore.YELLOW}with {len(warnings)} warning(s){Style.RESET_ALL}")
        return

    # Build failed: if no diagnostics were recognised (e.g. a linker error that
    # doesn't match the compiler patterns), show the tail of the raw output so
    # the actual failure is still visible.
    if not errors:
        tail = [ln for ln in combined.splitlines() if ln.strip()][-12:]
        for line in tail:
            _persist(job, f"{Style.DIM}{line}{Style.RESET_ALL}")

    summary = f"{Fore.RED}✖ Build failed{Style.RESET_ALL} with {len(errors)} error(s)"
    if warnings:
        summary += f" and {len(warnings)} warning(s)"
    _persist(job, summary)
    _persist(job, f"{Style.DIM}See {log_path} for the full log.{Style.RESET_ALL}")


def _persist(job, message: str):
    """Show build diagnostics so they survive past the job's live region."""
    if job is not None:
        job.persist(message)
    else:
        print(message)