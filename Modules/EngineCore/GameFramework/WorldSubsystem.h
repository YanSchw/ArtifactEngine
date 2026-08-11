#pragma once
#include "Object/Object.h"
#include "WorldSubsystem.gen.h"

class World;

/** A per-World singleton. Every WorldSubsystem subclass is instantiated once by each World,
*   which owns it for its entire lifetime and updates it before its Nodes. */
class WorldSubsystem : public Object {
public:
    ARTIFACT_CLASS();

    virtual void Initialize() {}
    virtual void WorldUpdate(float InDeltaTime) { (void)InDeltaTime; }
    virtual void Shutdown() {}

    World& GetWorld() const { return *m_World; }

private:
    World* m_World = nullptr;

    friend class World;
};
