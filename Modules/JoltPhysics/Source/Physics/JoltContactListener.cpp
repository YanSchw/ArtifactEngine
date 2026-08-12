#include "JoltContactListener.h"
#include "GameFramework/PhysicsBodyNode.h"
#include "GameFramework/PhysicsWorld.h"
#include <Jolt/Physics/Body/Body.h>

static uint64_t MakePairKey(uint32_t InBody1, uint32_t InBody2) {
    const uint32_t first = glm::min(InBody1, InBody2);
    const uint32_t second = glm::max(InBody1, InBody2);
    return ((uint64_t)first << 32) | (uint64_t)second;
}

void JoltContactListener::OnContactAdded(const JPH::Body& InBody1, const JPH::Body& InBody2, const JPH::ContactManifold& InManifold, JPH::ContactSettings& IoSettings) {
    (void)InManifold;
    (void)IoSettings;
    Enqueue(InBody1.GetID().GetIndexAndSequenceNumber(), InBody2.GetID().GetIndexAndSequenceNumber(), true);
}

void JoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& InSubShapePair) {
    Enqueue(InSubShapePair.GetBody1ID().GetIndexAndSequenceNumber(), InSubShapePair.GetBody2ID().GetIndexAndSequenceNumber(), false);
}

void JoltContactListener::Enqueue(uint32_t InBody1, uint32_t InBody2, bool InAdded) {
    const std::lock_guard<std::mutex> lock(m_Mutex);
    m_Events.Add({ InBody1, InBody2, InAdded });
}

bool JoltContactListener::UpdateContactCount(const Event& InEvent) {
    const uint64_t key = MakePairKey(InEvent.Body1, InEvent.Body2);
    const int32_t count = m_ContactCounts.GetOrDefault(key, 0);
    const int32_t next = glm::max(count + (InEvent.Added ? 1 : -1), 0);

    if (next > 0) {
        m_ContactCounts[key] = next;
    } else {
        m_ContactCounts.Remove(key);
    }
    return InEvent.Added ? count == 0 : next == 0;
}

void JoltContactListener::Dispatch(PhysicsWorld& InWorld) {
    Array<Event> events;
    {
        const std::lock_guard<std::mutex> lock(m_Mutex);
        events = m_Events;
        m_Events.Clear();
    }

    for (const Event& event : events) {
        if (!UpdateContactCount(event)) {
            continue;
        }

        Node3D* first = InWorld.GetNodeFromBody(event.Body1);
        Node3D* second = InWorld.GetNodeFromBody(event.Body2);
        if (first == nullptr || second == nullptr) {
            continue;
        }

        PhysicsBodyNode* firstBody = Cast<PhysicsBodyNode>(first);
        PhysicsBodyNode* secondBody = Cast<PhysicsBodyNode>(second);
        const bool firstIsTrigger = firstBody != nullptr && firstBody->IsTrigger();
        const bool secondIsTrigger = secondBody != nullptr && secondBody->IsTrigger();

        if (firstIsTrigger || secondIsTrigger) {
            if (firstIsTrigger) {
                if (event.Added) {
                    firstBody->OnTriggerEnter(second);
                } else {
                    firstBody->OnTriggerExit(second);
                }
            }
            if (secondIsTrigger) {
                if (event.Added) {
                    secondBody->OnTriggerEnter(first);
                } else {
                    secondBody->OnTriggerExit(first);
                }
            }
            continue;
        }

        if (firstBody == nullptr || secondBody == nullptr) {
            continue;
        }
        if (event.Added) {
            firstBody->OnCollisionEnter(secondBody);
            secondBody->OnCollisionEnter(firstBody);
        } else {
            firstBody->OnCollisionExit(secondBody);
            secondBody->OnCollisionExit(firstBody);
        }
    }
}
