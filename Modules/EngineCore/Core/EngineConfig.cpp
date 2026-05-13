#include "EngineConfig.h"

#include <filesystem>
#include "Log.h"
#include "Object/Pointer.h"
#include "Platform/Platform.h"

void EngineConfig::Initialize(const Array<String>& InArgs) {
    bool forceEditor = false;
    bool forceRuntime = false;

    for (const auto& arg : InArgs) {
        if (arg == "--Editor") forceEditor = true;
        if (arg == "--Runtime") forceRuntime = true;
    }

    if (forceEditor) {
        s_EngineClass = Class("EditorEngine");
    } else if (forceRuntime) {
        s_EngineClass = Class("GameEngine");
    } else {
        #if defined(AE_PACKAGED)
            s_EngineClass = Class("GameEngine");
        #else
            s_EngineClass = Class("EditorEngine");
        #endif
    }

    ResolvePaths();
}

void EngineConfig::ResolvePaths() {
    AE_INFO("Resolving Paths from CWD: {0}", std::filesystem::current_path().string());

#if defined(AE_PACKAGED)
    // In package builds use the Platform API
    s_ContentDir = Platform::GetContentDirectory();
#else
    // In non-package builds mount the Content dir from the CWD
    auto contentDirPath = std::filesystem::current_path();
    auto root = contentDirPath.root_path();
    while (true) {
        if (std::filesystem::is_directory(contentDirPath / "Content")) {
            s_ContentDir = (contentDirPath / "Content").string();
            break;
        }

        if (contentDirPath == root) {
            AE_ASSERT(false, "Content directory not found!");
            break;
        }

        contentDirPath = contentDirPath.parent_path();
    }
#endif

    AE_INFO("Resolved Content Directory: {0}", s_ContentDir);
}