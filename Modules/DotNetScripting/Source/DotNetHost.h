#pragma once
#include "Common/String.h"

class DotNetHost {
public:
    ~DotNetHost();

    bool Start(const String& InRuntimeConfigPath, const String& InDotNetRoot);
    bool LoadAssembly(const String& InAssemblyPath);
    void* GetFunction(const String& InTypeName, const String& InMethodName) const;

private:
    void* m_Library = nullptr;
    void* m_Context = nullptr;
    void* m_Close = nullptr;
    void* m_LoadAssembly = nullptr;
    void* m_GetFunctionPointer = nullptr;
};
