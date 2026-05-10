#include <iostream>

#include "Log.h"
#include "Assert.h"
#include "Engine.h"
#include "Platform/Platform.h"
#include "Common/Types.h"
#include "Common/Version.h"
#include "Core/Pointer.h"
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
    EnforceLinkingStaticLibraries();
    AE_INFO("Artifact Engine Version {0}", Version::GetVersionString());

    EngineConfig::Initialize(InArgs);

    SharedObjectPtr<Engine> engine = Object::Create<Engine>(EngineConfig::EngineClass());
    AE_ASSERT(engine, "Failed to create engine instance!");
    engine->Initialize();
    engine->MainLoop();
    engine->Shutdown();

    return 0;
}

int main(int argc, char** argv) {
    Array<String> args;
    for (int i = 0; i < argc; i++) {
        args.Add(String(argv[i]));
    }
    return ArtifactMain(args);
}