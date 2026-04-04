#include <iostream>

#include "Log.h"
#include "Engine.h"
#include "Platform/Platform.h"
#include "Common/Types.h"
#include "Common/Version.h"
#include "Core/Pointer.h"
#include <vector>
#include <string>

static void ModuleLinking() {
    std::vector<std::string> linkedModules;
    extern void __LinkModules(std::vector<std::string>& modules);
    __LinkModules(linkedModules);

    AE_INFO("Linked Modules:");
    for (const auto& module : linkedModules) {
        AE_INFO(" - {0}", module);
    }
}

int ArtifactMain() {
    ModuleLinking();

    AE_INFO("Artifact Engine Version {0}", Version::GetVersionString());

    SharedObjectPtr<Engine> engine = Object::Create<Engine>(Class("EditorEngine"));
    engine->Initialize();
    engine->MainLoop();
    engine->Shutdown();

    return 0;
}

int main() {
    return ArtifactMain();
}