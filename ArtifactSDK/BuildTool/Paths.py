import os

def get_sdk_path() -> str:
    """Returns the path to the Artifact SDK."""
    import BuildTool
    from pathlib import Path

    SRC_ROOT = Path(BuildTool.__file__).resolve().parent.parent
    return str(SRC_ROOT).replace("\\", "/")

def get_engine_path() -> str:
    """Returns the path to the Artifact Engine root directory."""
    import BuildTool
    from pathlib import Path

    SRC_ROOT = Path(BuildTool.__file__).resolve().parent.parent.parent
    return str(SRC_ROOT).replace("\\", "/")

def get_project_path() -> str:
    project_path = os.getcwd().replace("\\", "/")
    return project_path