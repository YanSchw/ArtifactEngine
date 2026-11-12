#include "DotNetHost.h"
#include "Core/Log.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
    #define DOTNET_CALLTYPE __cdecl
    static constexpr const char* s_HostFxrName = "hostfxr.dll";
#else
    #include <dlfcn.h>
    #define DOTNET_CALLTYPE
    #if defined(__APPLE__)
        static constexpr const char* s_HostFxrName = "libhostfxr.dylib";
    #else
        static constexpr const char* s_HostFxrName = "libhostfxr.so";
    #endif
#endif

using NativeChar = std::filesystem::path::value_type;
using InitializeForRuntimeConfigFn = int32_t(DOTNET_CALLTYPE*)(const NativeChar*, const void*, void**);
using GetRuntimeDelegateFn = int32_t(DOTNET_CALLTYPE*)(void*, int32_t, void**);
using CloseFn = int32_t(DOTNET_CALLTYPE*)(void*);
using LoadAssemblyFn = int32_t(DOTNET_CALLTYPE*)(const NativeChar*, void*, void*);
using GetFunctionPointerFn = int32_t(DOTNET_CALLTYPE*)(const NativeChar*, const NativeChar*, const NativeChar*, void*, void*, void**);

static constexpr int32_t s_GetFunctionPointerDelegate = 6;
static constexpr int32_t s_LoadAssemblyDelegate = 7;
static const NativeChar* const s_UnmanagedCallersOnly = (const NativeChar*)-1;

static void* LoadLibraryAt(const std::filesystem::path& InPath) {
#if defined(_WIN32)
    return (void*)::LoadLibraryW(InPath.c_str());
#else
    return dlopen(InPath.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

static void* FindSymbol(void* InLibrary, const char* InName) {
#if defined(_WIN32)
    return (void*)::GetProcAddress((HMODULE)InLibrary, InName);
#else
    return dlsym(InLibrary, InName);
#endif
}

static std::vector<std::filesystem::path> CandidateDotNetRoots() {
    std::vector<std::filesystem::path> roots;
    if (const char* root = std::getenv("DOTNET_ROOT")) {
        roots.emplace_back(root);
    }
#if defined(_WIN32)
    roots.emplace_back("C:/Program Files/dotnet");
#else
    if (const char* home = std::getenv("HOME")) {
        roots.emplace_back(std::filesystem::path(home) / ".dotnet");
    }
    roots.emplace_back("/usr/local/share/dotnet");
    roots.emplace_back("/usr/share/dotnet");
    roots.emplace_back("/usr/lib/dotnet");
    roots.emplace_back("/opt/homebrew/share/dotnet");
#endif
    return roots;
}

static std::filesystem::path FindHostFxr() {
    for (const std::filesystem::path& root : CandidateDotNetRoots()) {
        std::error_code error;
        const std::filesystem::path fxr = root / "host" / "fxr";
        if (!std::filesystem::is_directory(fxr, error)) {
            continue;
        }
        std::vector<std::filesystem::path> versions;
        for (const auto& entry : std::filesystem::directory_iterator(fxr, error)) {
            if (std::filesystem::exists(entry.path() / s_HostFxrName)) {
                versions.push_back(entry.path());
            }
        }
        if (!versions.empty()) {
            std::sort(versions.begin(), versions.end());
            return versions.back() / s_HostFxrName;
        }
    }
    return {};
}

DotNetHost::~DotNetHost() {
    if (m_Context && m_Close) {
        ((CloseFn)m_Close)(m_Context);
    }
}

bool DotNetHost::Start(const String& InRuntimeConfigPath) {
    const std::filesystem::path hostFxr = FindHostFxr();
    m_Library = hostFxr.empty() ? nullptr : LoadLibraryAt(hostFxr);
    if (!m_Library) {
        AE_WARN("No .NET runtime found; set DOTNET_ROOT to enable C# scripting");
        return false;
    }

    auto initialize = (InitializeForRuntimeConfigFn)FindSymbol(m_Library, "hostfxr_initialize_for_runtime_config");
    auto getDelegate = (GetRuntimeDelegateFn)FindSymbol(m_Library, "hostfxr_get_runtime_delegate");
    m_Close = FindSymbol(m_Library, "hostfxr_close");
    if (!initialize || !getDelegate || !m_Close) {
        AE_ERROR("{0} is missing the hosting entry points", hostFxr.string());
        return false;
    }

    const std::filesystem::path config(InRuntimeConfigPath);
    if (initialize(config.c_str(), nullptr, &m_Context) != 0 || !m_Context) {
        AE_ERROR("Could not initialize the .NET runtime from {0}", InRuntimeConfigPath);
        return false;
    }
    return getDelegate(m_Context, s_LoadAssemblyDelegate, &m_LoadAssembly) == 0
        && getDelegate(m_Context, s_GetFunctionPointerDelegate, &m_GetFunctionPointer) == 0;
}

bool DotNetHost::LoadAssembly(const String& InAssemblyPath) {
    const std::filesystem::path assembly(InAssemblyPath);
    return m_LoadAssembly && ((LoadAssemblyFn)m_LoadAssembly)(assembly.c_str(), nullptr, nullptr) == 0;
}

void* DotNetHost::GetFunction(const String& InTypeName, const String& InMethodName) const {
    void* function = nullptr;
    const std::filesystem::path type(InTypeName);
    const std::filesystem::path method(InMethodName);
    if (!m_GetFunctionPointer
        || ((GetFunctionPointerFn)m_GetFunctionPointer)(type.c_str(), method.c_str(), s_UnmanagedCallersOnly, nullptr, nullptr, &function) != 0) {
        AE_ERROR("Managed entry point {0}.{1} not found", InTypeName, InMethodName);
        return nullptr;
    }
    return function;
}
