
from pathlib import Path

from colorama import Fore, Style


class LintError(Exception):
    def __init__(self, message):
        # A rule may raise a single LintError (with a message) or pass a list of
        # LintErrors / messages to report several problems from one invocation.
        if isinstance(message, (list, tuple)):
            self.errors = [
                e if isinstance(e, LintError) else LintError(e)
                for e in message
            ]
            super().__init__(f"{len(self.errors)} lint errors")
        else:
            self.errors = [self]
            super().__init__(message)

def lint(*exts):
    exts = {e.lower() for e in exts}

    def decorator(func):
        func._lint_exts = exts
        return func

    return decorator


import importlib
import pkgutil
import inspect

def load_lint_functions(package_name="Lint.Rules"):
    functions = []

    package = importlib.import_module(package_name)

    for _, module_name, _ in pkgutil.walk_packages(package.__path__, package.__name__ + "."):
        module = importlib.import_module(module_name)

        for _, obj in inspect.getmembers(module, inspect.isfunction):
            if hasattr(obj, "_lint_exts"):
                functions.append(obj)

    return functions


def lint_files(files: list[str]) -> int:
    lint_functions = load_lint_functions()
    error_count = 0

    for file in files:
        if "ThirdParty" in file:
            continue

        path = Path(file)
        ext = path.suffix.lstrip(".").lower()

        try:
            content = path.read_text(encoding="utf-8")
        except Exception as e:
            print(f"{file}: failed to read ({e})")
            error_count += 1
            continue

        for func in lint_functions:
            if ext in func._lint_exts:
                try:
                    func(file, content)
                except LintError as e:
                    for err in e.errors:
                        print(f"{Fore.RED}[{func.__name__}] {err}{Style.RESET_ALL}")
                        error_count += 1

    return error_count