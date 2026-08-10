#include "Platform/Platform.h"
#include "Platform/Subprocess.h"

#include <filesystem>

// Everything the package ships is preloaded at the filesystem root by the emscripten runtime.
static constexpr const char* s_ResourceDirectory = "/";

PlatformType Platform::CurrentPlatform() {
    return PlatformType::Web;
}

Class Platform::GetDefaultRenderingAPIClass() {
    return Class("WebGLAPI");
}

Class Platform::GetMainSurfaceClass() {
    return Class("BrowserSurface");
}

String Platform::GetResourceDirectory() {
    return s_ResourceDirectory;
}

String Platform::GetContentDirectory() {
    return (std::filesystem::path(GetResourceDirectory()) / "Content").string();
}

Platform::TemporaryDirectory::TemporaryDirectory() {
    static uint32_t counter = 0;
    const std::filesystem::path candidate = std::filesystem::temp_directory_path() / ("tmp_" + std::to_string(counter++));

    std::error_code error;
    std::filesystem::create_directories(candidate, error);
    if (!error) {
        Path = candidate.string();
    }
}

Platform::TemporaryDirectory::~TemporaryDirectory() {
    if (!Path.empty()) {
        std::error_code error;
        std::filesystem::remove_all(Path, error);
    }
}

SubprocessResult Subprocess::Run(const String& InCommand) {
    AE_WARN("Cannot run '{0}': the browser has no subprocesses", InCommand);
    return SubprocessResult{ -1, "", "the browser has no subprocesses" };
}
