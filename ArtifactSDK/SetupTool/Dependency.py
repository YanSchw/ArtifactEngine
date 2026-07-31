"""The dependency model behind `artifact setup`.

A dependency knows two things: how to detect whether it is present on this
machine (``check``) and how to install it without any GUI interaction
(``install``). Which dependencies exist is a property of the *development*
platform, not of the build target — see ``SetupTool.Setup.get_dependencies``.
"""

from SDK.Job import JobError


class SetupError(JobError):
    """Raised when a dependency cannot be checked or installed."""


class CheckResult:
    """Outcome of a dependency check.

    ``detail`` is a short human-readable string: the detected version or path
    when the dependency is satisfied, the reason it is considered missing
    otherwise.
    """

    def __init__(self, satisfied: bool, detail: str = ""):
        self.satisfied = satisfied
        self.detail = detail

    @staticmethod
    def found(detail: str = "") -> "CheckResult":
        return CheckResult(True, detail)

    @staticmethod
    def missing(detail: str = "not found") -> "CheckResult":
        return CheckResult(False, detail)


class Dependency:
    """One development dependency.

    ``key`` is what the user types (``artifact setup vulkan``); ``aliases`` are
    additional spellings accepted for it. The same ``key`` is reused across
    platforms for the equivalent dependency (``toolchain`` is AppleClang on
    macOS, MSVC on Windows and GCC on Linux) so scripts stay platform-agnostic.
    """

    key = ""
    name = ""
    aliases = ()

    def check(self) -> CheckResult:
        raise NotImplementedError

    def install(self):
        raise NotImplementedError

    def matches(self, requested: str) -> bool:
        requested = requested.lower()
        return requested == self.key or requested in self.aliases
