#include "GameInstance.h"
#include "World.h"

void GameInstance::Update(double InDeltatime) {
    // for (auto& It : m_GameModules) {
    //     It->Update(deltaTime);
    // }
    if (GetCurrentWorld()) {
        GetCurrentWorld()->Update(InDeltatime);

        // if (GetCurrentWorld()->GetMainCamera()) {
        //     //GetEngine()->GetRenderPipeline()->Render(*m_Framebuffer.get(), *GetCurrentWorld(), *GetCurrentWorld()->GetMainCamera());
        // }
    }
}

Array<SharedObjectPtr<World>> GameInstance::GetAllWorld() {
    return m_Worlds;
}

SharedObjectPtr<World> GameInstance::GetCurrentWorld() const {
    return m_CurrentWorld;
}

void GameInstance::SetCurrentWorld(World* InWorld) {
    for (const auto& It : m_Worlds) {
        if (It == InWorld) {
            m_CurrentWorld = InWorld;
            return;
        }
    }
    AE_ASSERT(false, "A World not managed by GameInstance may never become the new current world!");
}