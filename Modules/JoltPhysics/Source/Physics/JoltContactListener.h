#pragma once
#include "Common/Array.h"
#include "Common/Map.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <mutex>

class PhysicsWorld;

class JoltContactListener final : public JPH::ContactListener {
public:
    virtual void OnContactAdded(const JPH::Body& InBody1, const JPH::Body& InBody2, const JPH::ContactManifold& InManifold, JPH::ContactSettings& IoSettings) override;
    virtual void OnContactRemoved(const JPH::SubShapeIDPair& InSubShapePair) override;

    void Dispatch(PhysicsWorld& InWorld);

private:
    struct Event {
        uint32_t Body1 = 0;
        uint32_t Body2 = 0;
        bool Added = false;
    };

    void Enqueue(uint32_t InBody1, uint32_t InBody2, bool InAdded);
    bool UpdateContactCount(const Event& InEvent);

    Array<Event> m_Events;
    Map<uint64_t, int32_t> m_ContactCounts;
    std::mutex m_Mutex;
};
