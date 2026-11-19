import hashlib
import os
import shutil
from functools import cache

from BuildTool.Module import expand_indirect_module_dependencies
from SDK.Paths import get_engine_path, get_sdk_path
from SDK.Platforms import PlatformType, get_platform

PREBUILT_DIR_NAME = "Prebuilt"
SOURCE_SUFFIXES = (".h", ".hh", ".hpp", ".hxx", ".inl", ".ipp", ".c", ".cc", ".cpp", ".cxx", ".m", ".mm", "Module.json")


@cache
def _hash_files(directory: str, suffixes: tuple[str, ...]) -> str:
    digest = hashlib.sha1()
    for root, dirs, files in os.walk(directory):
        dirs.sort()
        for name in sorted(files):
            if name.endswith(suffixes):
                path = os.path.join(root, name)
                digest.update(os.path.relpath(path, directory).encode())
                with open(path, "rb") as f:
                    digest.update(f.read())
    return digest.hexdigest()


def _module_dirs(modules_dir: str) -> dict[str, str]:
    return {name: f"{modules_dir}/{name}" for name in os.listdir(modules_dir)
            if os.path.isfile(f"{modules_dir}/{name}/Module.json")}


def _subdirectories(directory: str) -> list[os.DirEntry]:
    return [entry for entry in os.scandir(directory) if entry.is_dir()] if os.path.isdir(directory) else []


def _prebuilt_dir() -> str:
    return f"{get_engine_path()}/Binaries/{PREBUILT_DIR_NAME}"


@cache
def get_module_hash(module_dir: str) -> str:
    module_dirs = _module_dirs(os.path.dirname(module_dir))
    digest = hashlib.sha1(_hash_files(f"{get_sdk_path()}/HeaderTool", (".py",)).encode())
    for name in sorted(expand_indirect_module_dependencies(module_dirs, [os.path.basename(module_dir)]) & module_dirs.keys()):
        digest.update(_hash_files(module_dirs[name], SOURCE_SUFFIXES).encode())
    return digest.hexdigest()[:16]


def get_variant_name(args) -> str:
    return f"{args.configuration}_{'Packaged' if getattr(args, 'packaged', False) else 'NonPackaged'}"


def get_prebuilt_library(module_name: str, module_dir: str, args) -> str:
    target = f"{args.target}Simulator" if getattr(args, "simulator", False) else args.target
    library = f"{module_name}.lib" if get_platform(args.target) == PlatformType.Win64 else f"lib{module_name}.a"
    return f"{_prebuilt_dir()}/{module_name}/{get_module_hash(module_dir)}/{target}/{get_variant_name(args)}/{library}"


def remove_stale_prebuilds():
    for module in _subdirectories(_prebuilt_dir()):
        module_dir = f"{get_engine_path()}/Modules/{module.name}"
        current_hash = get_module_hash(module_dir) if os.path.isfile(f"{module_dir}/Module.json") else None
        for prebuild in _subdirectories(module.path):
            if prebuild.name != current_hash:
                shutil.rmtree(prebuild.path)
