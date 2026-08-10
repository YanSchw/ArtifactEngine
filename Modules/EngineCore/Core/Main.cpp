#include <iostream>

#include "Log.h"
#include "Assert.h"
#include "MainThread.h"
#include "Engine.h"
#include "Platform/Platform.h"
#include "Common/Types.h"
#include "Common/Version.h"
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Core/EngineConfig.h"
#include <vector>
#include <string>

// a call to this function is necessary to ensure that the static libraries are linked in,
// otherwise the linker may exclude them since they are not directly referenced and only used via reflection
static void EnforceLinkingStaticLibraries() {
    std::vector<std::string> linkedModules;
    extern void __LinkModules(std::vector<std::string>& modules);
    __LinkModules(linkedModules);

    AE_INFO("Linked Modules:");
    for (const auto& module : linkedModules) {
        AE_INFO(" - {0}", module);
    }
}

int ArtifactMain(const Array<String>& InArgs) {
    AE_INFO("Artifact Engine Version {0}", Version::GetVersionString());
    MainThread::Register();
    EnforceLinkingStaticLibraries();

    EngineConfig::Initialize(InArgs);

    // Owned for the lifetime of the process: a host that drives the frame loop
    // itself leaves ArtifactMain before the engine is finished with.
    static SharedObjectPtr<Engine> engine = Object::Create<Engine>(EngineConfig::EngineClass());
    AE_ASSERT(engine, "Failed to create engine instance!");
    engine->Initialize();
    engine->MainLoop();
    engine->Shutdown();

    return 0;
}
