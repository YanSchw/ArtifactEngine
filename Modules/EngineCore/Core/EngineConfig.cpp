#include "EngineConfig.h"

#include <filesystem>
#include "Log.h"
#include "Object/Pointer.h"
#include "Common/ConfigVar.h"
#include "Common/UUID.h"
#include "Platform/Platform.h"

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