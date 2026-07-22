#include "EngineConfig.h"

#include <filesystem>
#include <vector>
#include <utility>
#include "Log.h"
#include "Object/Pointer.h"
#include "Common/ConfigVar.h"
#include "Common/UUID.h"
#include "Platform/Platform.h"
#include "Platform/FileIO.h"

static Map<String, ConfigVar> s_ConfigVars;

void EngineConfig::Initialize(const Array<String>& InArgs) {
    LoadConfigVars(InArgs);
    ResolvePaths();
}

void EngineConfig::LoadConfigVars(const Array<String>& InArgs) {
    // 1. Load default ConfigVars
    SetConfigVar("EngineClass", IsPackagedBuild() ? "GameEngine" : "EditorEngine");
    SetConfigVar("RenderPipeline", "ArtifactRenderPipeline");

    // 2. Load frozen ConfigVars from packaged build
#if defined(AE_PACKAGED)
    // TODO: Implement
#endif

    // 3. Load ConfigVars from config file
    // TODO: Implement

    // 4. Load ConfigVars from environment variables
    // TODO: Implement

    // 5. Load ConfigVars from command line args
    for (const String& arg : InArgs) {
        if (arg.starts_with("-")) {
            const size_t equalsIndex = arg.find('=');
            if (equalsIndex != String::npos) {
                const String name = arg.substr(1, equalsIndex - 1);
                const String value = arg.substr(equalsIndex + 1);
                if (!SetConfigVar(name, value)) {
                    AE_ERROR("Failed to set ConfigVar '{0}' from command line arg '{1}'", name, arg);
                }
            }
        }
    }
}

bool EngineConfig::SetConfigVar(const String& InName, const String& InValue) {
    if (s_ConfigVars.ContainsKey(InName)) {
        return s_ConfigVars[InName].Update(InValue);
    } else {
        s_ConfigVars[InName] = ConfigVar(InValue);
        return true;
    }
}

void EngineConfig::ResolvePaths() {
#if !defined(AE_PACKAGED)
    // In non-package builds mount the Content dirs
    std::vector<std::pair<std::string, std::string>> mounts;
    extern void __MountContentDirs(std::vector<std::pair<std::string, std::string>>& outMounts);
    __MountContentDirs(mounts);
    for (const auto& [key, dir] : mounts) {
        MountContent(String(key), String(dir));
    }
#endif
}

void EngineConfig::MountContent(const String& InKey, const String& InDir) {
    AE_INFO("Mounted content '{0}' -> {1}", InKey, InDir);
    if (!s_ContentMounts.ContainsKey(InKey)) {
        s_ContentMountKeys.Add(InKey);
    }
    s_ContentMounts[InKey] = InDir;
}

String EngineConfig::GetEngineContentDir() {
    return GetContentDir("EngineContent");
}

String EngineConfig::GetProjectContentDir() {
    if (!s_ContentMounts.ContainsKey("ProjectContent")) {
        return GetEngineContentDir();
    }
    return GetContentDir("ProjectContent");
}

String EngineConfig::GetContentDir(const String& InKey) {
    if (IsPackagedBuild()) {
        return Platform::GetContentDirectory();
    }
    if (s_ContentMounts.ContainsKey(InKey)) {
        return s_ContentMounts[InKey];
    }
    AE_ERROR("No content mounted for key '{0}'", InKey);
    return "";
}

Array<String> EngineConfig::GetContentMountDirs() {
    Array<String> dirs;
    if (IsPackagedBuild()) {
        dirs.Add(Platform::GetContentDirectory());
        return dirs;
    }
    for (const String& key : s_ContentMountKeys) {
        // Distinct keys can map to the same dir;
        // keep the list unique so files aren't scanned/resolved twice.
        const String& dir = s_ContentMounts[key];
        if (!dirs.Contains(dir)) {
            dirs.Add(dir);
        }
    }
    return dirs;
}

Array<String> EngineConfig::GetContentMountKeys() {
    return s_ContentMountKeys;
}

String EngineConfig::ResolveContentPath(const String& InRelativePath) {
    Array<String> dirs = GetContentMountDirs();
    for (const String& dir : dirs) {
        String candidate = dir + InRelativePath;
        if (FileIO::FileExists(candidate)) {
            return candidate;
        }
    }
    // Fall back to the first mount so callers still get a usable path (e.g. for error messages).
    if (dirs.Size() > 0) {
        return dirs[0] + InRelativePath;
    }
    return InRelativePath;
}

bool EngineConfig::IsPackagedBuild() {
#if defined(AE_PACKAGED)
    return true;
#else
    return false;
#endif
}


template<>
String EngineConfig::GetConfigVar<String>(const String& InName) {
    if (s_ConfigVars.ContainsKey(InName)) {
        return s_ConfigVars[InName].As<String>("");
    }
    return "";
}
template<>
int EngineConfig::GetConfigVar<int>(const String& InName) {
    if (s_ConfigVars.ContainsKey(InName)) {
        return s_ConfigVars[InName].As<int>(0);
    }
    return 0;
}
template<>
bool EngineConfig::GetConfigVar<bool>(const String& InName) {
    if (s_ConfigVars.ContainsKey(InName)) {
        return s_ConfigVars[InName].As<bool>(false);
    }
    return false;
}
template<>
Class EngineConfig::GetConfigVar<Class>(const String& InName) {
    if (s_ConfigVars.ContainsKey(InName)) {
        return Class(s_ConfigVars[InName].As<String>(""));
    }
    return Class("");
}
template<>
UUID EngineConfig::GetConfigVar<UUID>(const String& InName) {
    if (s_ConfigVars.ContainsKey(InName)) {
        String uuidStr = s_ConfigVars[InName].As<String>("");
        if (uuidStr.empty()) {
            return UUID::INVALID;
        }
        return UUID::FromString(uuidStr);
    }
    return UUID::INVALID;
}

Class EngineConfig::EngineClass() {
    return GetConfigVar<Class>("EngineClass");
}

Class EngineConfig::RenderPipelineClass() {
    return GetConfigVar<Class>("RenderPipeline");
}