#include <iostream>

#include "Log.h"
#include "Engine.h"
#include "Platform.h"
#include "Common/Types.h"
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

    #if VERSION_PATCH == None
    AE_INFO("Artifact Engine Version {0}.{1}", VERSION_MAJOR, VERSION_MINOR);
    #else
    AE_INFO("Artifact Engine Version {0}.{1}.{2}", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    #endif

    Engine* engine = Object::Create(Class("EditorEngine"))->As<Engine>();
    engine->Initialize();
    engine->MainLoop();
    engine->Shutdown();
    delete engine;

    return 0;
}

int main() {
    return ArtifactMain();
}