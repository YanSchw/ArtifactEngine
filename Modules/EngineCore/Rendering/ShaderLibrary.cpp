#include "ShaderLibrary.h"

#include "Core/EngineConfig.h"
#include "Platform/FileIO.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderSource.h"
#include "Rendering/ShaderTemplate.h"
#include "Serialization/ChunkedBinary.h"

#include <filesystem>
#include <format>

static constexpr const char* s_ShaderExtension = ".glsl";
static constexpr uint32_t s_HeaderChunk = 0;
static constexpr uint32_t s_FirstEntryChunk = 1;

static Map<String, SharedObjectPtr<Shader>> s_Shaders;
static Map<String, String> s_RegisteredSources;
static Map<String, CompiledShader> s_CookedShaders;
static Array<String> s_KeyOrder;
static bool s_Initialized = false;

static String NormalizeKey(const String& InKey) {
    if (InKey.empty() || InKey[0] == '/' || InKey.find('/') == String::npos) {
        return InKey;
    }
    return "/" + InKey;
}

static ShaderAPI GetActiveShaderAPI() {
    RenderingAPI* renderingAPI = RenderingAPI::GetInstance();
    return renderingAPI ? renderingAPI->GetShaderAPI() : ShaderAPI::Unknown;
}

static void TrackKey(const String& InKey) {
    if (!s_KeyOrder.Contains(InKey)) {
        s_KeyOrder.Add(InKey);
    }
}

void ShaderLibrary::Initialize() {
    if (s_Initialized) {
        return;
    }
    s_Initialized = true;

    if (!EngineConfig::IsPackagedBuild()) {
        return;
    }

    if (!LoadCookedLibrary()) {
        AE_ERROR("Failed to load the cooked shader library; no shaders are available");
        return;
    }

    for (const auto& [key, compiled] : s_CookedShaders) {
        if (SharedObjectPtr<Shader> shader = Shader::Create(compiled)) {
            s_Shaders[key] = shader;
            TrackKey(key);
        } else {
            AE_ERROR("Failed to create shader '{0}' from the cooked shader library", key);
        }
    }

    AE_INFO("Shader library loaded {0} shaders", s_Shaders.Size());
}

void ShaderLibrary::Shutdown() {
    s_Shaders.Clear();
    s_CookedShaders.Clear();
    s_RegisteredSources.Clear();
    s_KeyOrder.Clear();
    s_Initialized = false;
}

bool ShaderLibrary::LoadCookedLibrary() {
    const String path = Platform::GetContentDirectory() + "/" + CookedFileName;

    SharedObjectPtr<ChunkedBinary> binary = ChunkedBinary::LoadFromFile(path);
    if (!binary) {
        AE_ERROR("Shader library not found at '{0}'", path);
        return false;
    }

    const ShaderAPI activeAPI = GetActiveShaderAPI();

    ChunkReader header = binary->GetChunk(s_HeaderChunk);
    uint32_t apiCount = 0;
    header >> apiCount;

    uint32_t entryChunk = 0;
    uint32_t entryCount = 0;
    bool found = false;

    for (uint32_t i = 0; i < apiCount; i++) {
        uint32_t api = 0;
        uint32_t chunkId = 0;
        uint32_t count = 0;
        header >> api;
        header >> chunkId;
        header >> count;

        if ((ShaderAPI)api == activeAPI) {
            entryChunk = chunkId;
            entryCount = count;
            found = true;
        }
    }

    if (!found) {
        AE_ERROR("Shader library holds no shaders for the active rendering API");
        return false;
    }

    ChunkReader entries = binary->GetChunk(entryChunk);
    for (uint32_t i = 0; i < entryCount; i++) {
        String key;
        entries >> key;

        CompiledShader compiled;
        if (!compiled.Deserialize(entries)) {
            AE_ERROR("Shader library entry '{0}' is corrupt", key);
            return false;
        }

        s_CookedShaders[key] = compiled;
    }

    return true;
}

SharedObjectPtr<Shader> ShaderLibrary::Find(const String& InKey) {
    const String key = NormalizeKey(InKey);
    return s_Shaders.ContainsKey(key) ? s_Shaders[key] : nullptr;
}

SharedObjectPtr<Shader> ShaderLibrary::CreateShader(const String& InKey) {
    const String key = NormalizeKey(InKey);

    if (s_Shaders.ContainsKey(key)) {
        return s_Shaders[key];
    }

    if (EngineConfig::IsPackagedBuild()) {
        AE_ERROR("Shader '{0}' is not in the cooked shader library", key);
        return nullptr;
    }

    CompiledShader compiled;
    String error;
    if (!CompileForKey(key, GetActiveShaderAPI(), compiled, error)) {
        AE_ERROR("Failed to compile shader '{0}':\n{1}", key, error);
        return nullptr;
    }

    SharedObjectPtr<Shader> shader = Shader::Create(compiled);
    if (!shader) {
        AE_ERROR("Failed to create shader '{0}'", key);
        return nullptr;
    }

    s_Shaders[key] = shader;
    TrackKey(key);
    return shader;
}

bool ShaderLibrary::Reload(const String& InKey, String& OutError) {
    const String key = NormalizeKey(InKey);

    if (EngineConfig::IsPackagedBuild()) {
        OutError = "shaders cannot be recompiled in a packaged build";
        return false;
    }

    CompiledShader compiled;
    if (!CompileForKey(key, GetActiveShaderAPI(), compiled, OutError)) {
        return false;
    }

    if (RenderingAPI* renderingAPI = RenderingAPI::GetInstance()) {
        renderingAPI->WaitIdle();
    }

    if (!s_Shaders.ContainsKey(key)) {
        SharedObjectPtr<Shader> shader = Shader::Create(compiled);
        if (!shader) {
            OutError = std::format("failed to create shader '{0}'", key);
            return false;
        }
        s_Shaders[key] = shader;
        TrackKey(key);
        return true;
    }

    s_Shaders[key]->Reload(compiled);
    Pipeline::InvalidateAll();
    return true;
}

void ShaderLibrary::RegisterSource(const String& InKey, const String& InSource) {
    const String key = NormalizeKey(InKey);
    s_RegisteredSources[key] = InSource;
    TrackKey(key);
}

bool ShaderLibrary::CompileForKey(const String& InKey, ShaderAPI InAPI, CompiledShader& OutCompiled, String& OutError) {
    ShaderCompiler* compiler = ShaderCompiler::Get(InAPI);
    if (!compiler) {
        OutError = "no shader compiler available for the active rendering API";
        return false;
    }

    ShaderSource source;
    const bool preprocessed = s_RegisteredSources.ContainsKey(InKey)
        ? ShaderSource::PreprocessSource(InKey, s_RegisteredSources[InKey], source, OutError)
        : ShaderSource::Preprocess(InKey, source, OutError);

    if (!preprocessed) {
        return false;
    }

    return compiler->Compile(source, OutCompiled, OutError);
}

Array<String> ShaderLibrary::DiscoverShaderKeys() {
    Array<String> keys;

    for (const String& mountDir : EngineConfig::GetContentMountDirs()) {
        const std::filesystem::path mountPath(mountDir);
        for (const String& file : FileIO::ListFilesInDirectory(mountDir, true)) {
            const std::filesystem::path filePath(file);
            if (filePath.extension().string() != s_ShaderExtension) {
                continue;
            }

            if (ShaderTemplate::IsTemplateSource(FileIO::ReadFileToString(file))) {
                continue;
            }

            std::error_code error;
            const std::filesystem::path relative = std::filesystem::relative(filePath, mountPath, error);
            if (error) {
                continue;
            }

            String key = "/" + relative.generic_string();
            if (!keys.Contains(key)) {
                keys.Add(key);
            }
        }
    }

    for (const String& key : s_KeyOrder) {
        if (s_RegisteredSources.ContainsKey(key) && !keys.Contains(key)) {
            keys.Add(key);
        }
    }

    return keys;
}

bool ShaderLibrary::Cook(const String& InOutputDirectory, PlatformType InTargetPlatform) {
    const Array<ShaderAPI> apis = ShaderCompiler::GetAPIsForPlatform(InTargetPlatform);
    if (apis.IsEmpty()) {
        AE_ERROR("No shader compiler supports the target platform");
        return false;
    }

    const Array<String> keys = DiscoverShaderKeys();
    bool succeeded = true;

    ChunkWriter headerChunk;
    Array<ChunkWriter> entryChunks;
    Array<uint32_t> entryCounts;

    headerChunk << (uint32_t)apis.Size();

    for (int32_t apiIndex = 0; apiIndex < apis.Size(); apiIndex++) {
        const ShaderAPI api = apis[apiIndex];
        ChunkWriter entryChunk;
        uint32_t entryCount = 0;

        for (int32_t i = 0; i < keys.Size(); i++) {
            const String& key = keys[i];
            AE_INFO("Cooking shader {0}/{1}: {2}", i + 1, keys.Size(), key);

            CompiledShader compiled;
            String error;
            if (!CompileForKey(key, api, compiled, error)) {
                AE_ERROR("Failed to cook shader '{0}':\n{1}", key, error);
                succeeded = false;
                continue;
            }

            entryChunk << key;
            compiled.Serialize(entryChunk);
            entryCount++;
        }

        headerChunk << (uint32_t)api;
        headerChunk << (uint32_t)(s_FirstEntryChunk + apiIndex);
        headerChunk << entryCount;

        entryChunks.Add(entryChunk);
        entryCounts.Add(entryCount);
    }

    ChunkedBinary binary;
    binary.AddChunk(s_HeaderChunk, headerChunk);
    for (int32_t i = 0; i < entryChunks.Size(); i++) {
        binary.AddChunk(s_FirstEntryChunk + i, entryChunks[i]);
    }

    if (!binary.SaveToFile(InOutputDirectory + "/" + CookedFileName)) {
        AE_ERROR("Failed to write the cooked shader library");
        return false;
    }

    return succeeded;
}
