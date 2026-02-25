#include <iostream>

#include "Log.h"
#include "Engine.h"
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