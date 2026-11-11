#pragma once
#include "GameFramework/WorldSubsystem.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "EventGraphSubsystem.gen.h"

class EventGraph;
class EventGraphInstance;

class EventGraphSubsystem : public WorldSubsystem {
public:
    ARTIFACT_CLASS();

    static EventGraph* FindGraphFor(Node& InNode);

    EventGraphInstance* FindInstance(const Node& InSelf) const;

    virtual void OnNodeBeginPlay(Node& InNode) override;
    virtual void OnNodeEndPlay(Node& InNode) override;
    virtual void WorldUpdate(float InDeltaTime) override;
    virtual void Shutdown() override;

private:
    Array<SharedObjectPtr<EventGraphInstance>> m_Instances;
};
