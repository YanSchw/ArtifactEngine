#pragma once
#include "Core/EngineSubsystem.h"
#include "ScriptCompiler.h"
#include "DotNetScripting.gen.h"

class DotNetScripting : public EngineSubsystem {
public:
    ARTIFACT_CLASS();

    virtual void Initialize() override;
    virtual void Tick(double InDeltaTime) override;

    void Reload();
    int32_t GetReloadCount() const { return m_ReloadCount; }

private:
    bool LoadAssemblies();
    size_t ScanSources() const;
    void ReportCompileErrors(const String& InOutput) const;

    Array<String> m_Assemblies;
    Array<String> m_WatchDirectories;
    String m_BuildCommand;
    ScriptCompiler m_Compiler;
    size_t m_SourceStamp = 0;
    double m_PollTimer = 0.0;
    int32_t m_ReloadCount = 0;
    bool m_Ready = false;
};
