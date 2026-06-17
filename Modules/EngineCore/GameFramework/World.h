#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "World.gen.h"

class Node;

/** Container for all Nodes during Gameplay */
class World final : public Object {
public:
    ARTIFACT_CLASS();

    void Update(double InDeltatime);

    Array<Node*> GetAllNodes() const;

    Node* Spawn(const Class& InClass);
    template<typename T>
    T* Spawn() {
        return Spawn(T::StaticClass())->template As<T>();
    }

private:
    void UnregisterNode(Node* node);
    void ReregisterNode(Node* node);

    void ResolveAllBeginPlayIssues();
    void ResolvePendingKills();

    void WorldUpdate(float deltaTime);
    void PrepareLocalUpdate();

private:
    Array<Node*> m_WorldNodes;
    Array<Node*> m_WorldUpdateNodes;
    Array<Node*> m_LocalUpdateNodes;

    /** Array of all nodes, that need to have BeginPlay() called. */
    Array<WeakObjectPtr<Node>> m_BeginPlayIssues;

    /** Array of all nodes, that will be killed at the end of the frame. 
    *   Kill happens outside of Gameplay Context. */
    Array<Node*> m_PendingKills;

    friend class Node;
};