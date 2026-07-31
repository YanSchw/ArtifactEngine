"""Process helpers for the setup checks and installers.

Installers run with their output attached to the terminal rather than inside a
``Job`` spinner: several of them prompt (sudo asks for a password, the Visual
Studio bootstrapper raises UAC), and a hidden prompt looks like a hang.
"""

import os
import shlex
import subprocess
import sys

from colorama import Fore, Style

from SetupTool.Dependency import SetupError


def format_command(command) -> str:
    parts = [str(part) for part in command]
    if sys.platform == "win32":
        return subprocess.list2cmdline(parts)
    return shlex.join(parts)


def sudo_prefix() -> list:
    """``sudo`` unless we already run as root (containers, CI images)."""
    if sys.platform == "win32" or os.geteuid() == 0:
        return []
    return ["sudo"]


def run_command(command, allowed_returncodes=(0,), allow_failure: bool = False, **kwargs) -> int:
    """Run an installer command, echoing it so the user sees what is happening.

    Returns the exit code. Unless ``allow_failure`` is set, an exit code outside
    ``allowed_returncodes`` raises ``SetupError``.
    """
    command = [str(part) for part in command]
    print(f"{Style.DIM}$ {format_command(command)}{Style.RESET_ALL}")
    try:
        returncode = subprocess.run(command, **kwargs).returncode
    except OSError as error:
        if allow_failure:
            return 1
        raise SetupError(f"Could not run '{command[0]}': {error}")
    if returncode not in allowed_returncodes and not allow_failure:
        raise SetupError(f"Command failed with exit code {returncode}: {format_command(command)}", returncode)
    return returncode


def command_output(command, combined: bool = False, **kwargs):
    """Run a command and return its trimmed output, or None if it fails.

    Tools disagree on which stream they report versions on, so ``combined``
    merges stdout and stderr and ignores the exit code.
    """
    try:
        result = subprocess.run([str(part) for part in command], capture_output=True,
                                text=True, errors="replace", timeout=120, **kwargs)
    except (OSError, subprocess.SubprocessError):
        return None
    if combined:
        return f"{result.stdout}\n{result.stderr}".strip()
    if result.returncode != 0:
        return None
    return result.stdout.strip() or result.stderr.strip()


def command_succeeds(command, **kwargs) -> bool:
    """Whether a command runs and exits successfully (for probes like pkg-config)."""
    try:
        result = subprocess.run([str(part) for part in command], capture_output=True,
                                text=True, errors="replace", timeout=120, **kwargs)
    except (OSError, subprocess.SubprocessError):
        return False
    return result.returncode == 0


def first_line(text) -> str:
    return text.splitlines()[0].strip() if text else ""


def step(message: str):
    print(f"{Fore.CYAN}>{Style.RESET_ALL} {message}")


def note(message: str):
    print(f"{Style.DIM}{message}{Style.RESET_ALL}")
