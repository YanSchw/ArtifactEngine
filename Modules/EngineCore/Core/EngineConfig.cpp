#include "EngineConfig.h"

#include <filesystem>
#include <vector>
#include <tuple>
#include <cstdlib>
#include "Log.h"
#include "Object/Pointer.h"
#include "Common/ConfigVar.h"
#include "ProjectSettings.h"
#include "Serialization/Ini.h"
#include "Common/UUID.h"
#include "Platform/Platform.h"
#include "Platform/FileIO.h"

static Map<String, ConfigVar> s_ConfigVars;

void EngineConfig::Initialize(const Array<String>& InArgs) {
    ResolvePaths();
    LoadConfigVars(InArgs);
}

void EngineConfig::LoadConfigVars(const Array<String>& InArgs) {
    // 1. Load default ConfigVars
    SetConfigVar("EngineClass", IsPackagedBuild() ? "GameEngine" : "EditorEngine");
    SetConfigVar("Fullscreen", IsPackagedBuild() ? "true" : "false");
    ProjectSettings::RegisterDefaults();

    // 2. Load frozen ConfigVars from packaged build
#if defined(AE_PACKAGED)
    std::vector<std::tuple<std::string, std::string, bool>> frozen;
    extern void __FrozenConfigVars(std::vector<std::tuple<std::string, std::string, bool>>& outVars);
    __FrozenConfigVars(frozen);
    for (const auto& [name, value, hasValue] : frozen) {
        if (hasValue) {
            SetConfigVar(String(name), String(value));
        }
        s_ConfigVars[String(name)].Freeze();
    }
#endif

    // 3. Load ConfigVars from config file
    LoadConfigFiles(GetConfigDir());
    LoadConfigFiles(GetUserConfigDir());

    // 4. Load ConfigVars from environment variables
    Array<String> names;
    for (const auto& [name, var] : s_ConfigVars) {
        names.Add(name);
    }
    for (const String& name : names) {
        if (const char* value = std::getenv(("AE_" + name).c_str())) {
            if (!SetConfigVar(name, value)) {
                AE_WARN("Ignoring environment variable AE_{0}: '{0}' is frozen", name);
            }
        }
    }

    // 5. Load ConfigVars from command line args
    for (const String& arg : InArgs) {
        if (arg.starts_with("-")) {
            const size_t equalsIndex = arg.find('=');
            if (equalsIndex != String::npos) {
                const String name = arg.substr(1, equalsIndex - 1);
                const String value = arg.substr(equalsIndex + 1);
                if (!SetConfigVar(name, value)) {
                    AE_ERROR("Ignoring command line arg '{0}': '{1}' is frozen", arg, name);
                }
            }
        }
    }
}

void EngineConfig::LoadConfigFiles(const String& InDir) {
    const std::filesystem::path dir = InDir;
    std::error_code error;
    Array<String> files;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir, error)) {
        if (entry.is_regular_file(error) && entry.path().extension() == ".ini") {
            files.Add(entry.path().string());
        }
    }
    files.Sort([](const String& InA, const String& InB) { return InA < InB; });

    for (const String& file : files) {
        for (const IniFile::Entry& entry : IniFile::Parse(FileIO::ReadFileToString(file)).GetEntries()) {
            if (!SetConfigVar(entry.Key, entry.Value)) {
                AE_WARN("Ignoring '{0}' in {1}: it is frozen", entry.Key, file);
            }
        }
        AE_INFO("Loaded config {0}", file);
    }
}

bool EngineConfig::SetConfigVar(const String& InName, const String& InValue) {
    if (!s_ConfigVars[InName].Update(InValue)) {
        return false;
    }
    ProjectSettings::OnConfigVarChanged(InName, InValue);
    return true;
}

String EngineConfig::GetConfigDir() {
    if (IsPackagedBuild()) {
        return (std::filesystem::path(Platform::GetContentDirectory()) / "Config").string();
    }
    return (std::filesystem::path(GetProjectDir()) / "Config").string();
}

String EngineConfig::GetUserConfigDir() {
    if (!IsPackagedBuild()) {
        return (std::filesystem::path(GetProjectDir()) / "Saved" / "Config").string();
    }
    const ConfigVar* projectNameVar = s_ConfigVars.Find("ProjectName");
    String projectName = projectNameVar ? projectNameVar->As<String>("") : String();
    if (projectName.empty() || projectName == "." || projectName == ".." || projectName.find_first_of("/\\:*?\"<>|") != String::npos) {
        AE_WARN("ProjectName '{0}' is not a valid file name; using 'Artifact'", projectName);
        projectName = "Artifact";
    }
    return (std::filesystem::path(Platform::GetUserConfigDirectory()) / projectName / "Config").string();
}

void EngineConfig::ResolvePaths() {
#if !defined(AE_PACKAGED)
    // In non-package builds mount the Content dirs
    std::vector<std::tuple<std::string, std::string, bool>> mounts;
    extern void __MountContentDirs(std::vector<std::tuple<std::string, std::string, bool>>& outMounts);
    __MountContentDirs(mounts);
    for (const auto& [key, dir, packaged] : mounts) {
        MountContent(String(key), String(dir), packaged);
    }
#endif
}

void EngineConfig::MountContent(const String& InKey, const String& InDir, bool InPackaged) {
    AE_INFO("Mounted content '{0}' -> {1}", InKey, InDir);
    if (!s_ContentMounts.ContainsKey(InKey)) {
        s_ContentMountKeys.Add(InKey);
    }
    s_ContentMounts[InKey] = InDir;
    if (!InPackaged && !s_NonPackagedMountKeys.Contains(InKey)) {
        s_NonPackagedMountKeys.Add(InKey);
    }
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

String EngineConfig::GetProjectDir() {
    if (IsPackagedBuild()) {
        return "";
    }
    return std::filesystem::path(GetProjectContentDir()).parent_path().string();
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

Array<String> EngineConfig::GetPackagedContentMountDirs() {
    Array<String> dirs = GetContentMountDirs();
    for (const String& key : s_NonPackagedMountKeys) {
        dirs.Remove(s_ContentMounts[key]);
    }
    return dirs;
}

Array<String> EngineConfig::GetContentMountKeys() {
    return s_ContentMountKeys;
}

bool EngineConfig::IsPackagedContentPath(const String& InPath) {
    const String path = std::filesystem::path(InPath).generic_string();
    for (const String& key : s_NonPackagedMountKeys) {
        const String prefix = std::filesystem::path(s_ContentMounts[key]).generic_string() + "/";
        if (path.compare(0, prefix.size(), prefix) == 0) {
            return false;
        }
    }
    return true;
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

String EngineConfig::ToContentPath(const String& InFilePath) {
    const String path = std::filesystem::path(InFilePath).lexically_normal().generic_string();
    for (const String& dir : GetContentMountDirs()) {
        const String prefix = std::filesystem::path(dir).lexically_normal().generic_string() + "/";
        if (path.compare(0, prefix.size(), prefix) == 0) {
            return path.substr(prefix.size() - 1);
        }
    }
    return String();
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
float EngineConfig::GetConfigVar<float>(const String& InName) {
    if (s_ConfigVars.ContainsKey(InName)) {
        return s_ConfigVars[InName].As<float>(0.0f);
    }
    return 0.0f;
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
    return GetConfigVar<Class>("RenderPipelineClass");
}
