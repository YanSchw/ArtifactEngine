
import os
import json
from typing import List, Optional, Set

class ArtifactModule:
    """
    Represents an Artifact Engine module with metadata from Module.json.
    Provides smart defaults for missing fields and utilities for module management.
    """

    def __init__(
        self,
        name: str,
        path: str,
        ImportModules: Optional[List[str]] = None,
        TargetPlatforms: Optional[List[str]] = None,
        SourceDirectories: Optional[List[str]] = None,
        IncludePaths: Optional[List[str]] = None,
        ExportIncludePaths: Optional[List[str]] = None,
        AddAdditionalCMakeProjects: Optional[List[str]] = None,
        LinkLibraries: Optional[List[str]] = None,
        MountContentDir: Optional[str] = None,
    ):
        """
        Initialize an ArtifactModule with metadata.

        Args:
            name: The module name
            path: The module path
            ImportModules: Modules this module depends on
            TargetPlatforms: Platforms this module supports
            SourceDirectories: Directories containing source files
            IncludePaths: Include directories for this module
            ExportIncludePaths: Include directories exported to dependents
            AddAdditionalCMakeProjects: Additional CMake projects to include
            LinkLibraries: System link libraries/flags passed straight to the linker
            MountContentDir: Module-local content directory to mount at runtime (relative to the module)
        """
        self.name = name
        self.path = path
        self.ImportModules = ImportModules if ImportModules is not None else []
        self.TargetPlatforms = TargetPlatforms if TargetPlatforms is not None else []
        self.SourceDirectories = SourceDirectories if SourceDirectories is not None else None
        self.IncludePaths = IncludePaths if IncludePaths is not None else []
        self.ExportIncludePaths = ExportIncludePaths if ExportIncludePaths is not None else []
        self.AddAdditionalCMakeProjects = AddAdditionalCMakeProjects if AddAdditionalCMakeProjects is not None else []
        self.LinkLibraries = LinkLibraries if LinkLibraries is not None else []
        self.MountContentDir = MountContentDir

    @classmethod
    def load_from_json(cls, module_path: str) -> "ArtifactModule":
        """
        Load a module from its Module.json file.

        Args:
            module_path: Path to the module directory

        Returns:
            ArtifactModule instance with data from Module.json

        Raises:
            FileNotFoundError: If Module.json doesn't exist
            json.JSONDecodeError: If Module.json is invalid
        """
        module_json_path = os.path.join(module_path, "Module.json")
        
        if not os.path.exists(module_json_path):
            raise FileNotFoundError(f"Module.json not found at {module_json_path}")
        
        with open(module_json_path, "r") as f:
            data = json.load(f)
        
        # Extract module name from path
        module_name = os.path.basename(module_path.rstrip(os.sep))
        
        return cls(
            name=module_name,
            path=module_path,
            ImportModules=data.get("ImportModules", None),
            TargetPlatforms=data.get("TargetPlatforms", None),
            SourceDirectories=data.get("SourceDirectories", None),
            IncludePaths=data.get("IncludePaths", None),
            ExportIncludePaths=data.get("ExportIncludePaths", None),
            AddAdditionalCMakeProjects=data.get("AddAdditionalCMakeProjects", None),
            LinkLibraries=data.get("LinkLibraries", None),
            MountContentDir=data.get("MountContentDir", None),
        )

    def supports_platform(self, platform: str) -> bool:
        """Check if this module supports a specific platform."""
        return platform in self.TargetPlatforms if len(self.TargetPlatforms) > 0 else True

    def get_source_files_pattern(self) -> List[str]:
        """Get glob patterns for source files."""
        return [f"{sd}/*.cpp" for sd in self.SourceDirectories]

    def to_dict(self) -> dict:
        """Convert module to dictionary representation."""
        return {
            "ImportModules": self.ImportModules,
            "TargetPlatforms": self.TargetPlatforms,
            "SourceDirectories": self.SourceDirectories,
            "IncludePaths": self.IncludePaths,
            "ExportIncludePaths": self.ExportIncludePaths,
            "AddAdditionalCMakeProjects": self.AddAdditionalCMakeProjects,
            "LinkLibraries": self.LinkLibraries,
            "MountContentDir": self.MountContentDir,
        }
    
    def write_to_json(self):
        """Write the module metadata back to Module.json."""
        from SDK.Util import smart_open
        module_json_path = os.path.join(self.path, "Module.json")
        with smart_open(module_json_path) as f:
            dict_data = self.to_dict()
            # remove keys with None values or empty lists for cleaner JSON
            dict_data = {k: v for k, v in dict_data.items() if v is not None and v != []}
            json.dump(dict_data, f, indent=4)

    def __repr__(self) -> str:
        return f"ArtifactModule(name={self.name}, path={self.path}, dependencies={self.ImportModules})"