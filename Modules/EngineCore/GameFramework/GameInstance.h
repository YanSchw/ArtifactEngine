#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "GameInstance.gen.h"

class World;

/** GameInstance is created and managed by the Engine. All GameState and GameFlow happends here. */
class GameInstance final : public Object {
public:
    ARTIFACT_CLASS();

    void Update(double InDeltatime);

    Array<SharedObjectPtr<World>> GetAllWorld();
    SharedObjectPtr<World> GetCurrentWorld() const;
    void SetCurrentWorld(World* InWorld);

private:
    SharedObjectPtr<World> m_CurrentWorld;
    Array<SharedObjectPtr<World>> m_Worlds;
};