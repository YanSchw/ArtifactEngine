#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "World.gen.h"

class Node;
class CameraNode;
class Scene;
class SceneRootNode;
class WorldSubsystem;

/** Container for all Nodes during Gameplay */
class World final : public Object {
public:
    ARTIFACT_CLASS();

    World();
    virtual ~World();

    void Update(double InDeltatime);

    WorldSubsystem* GetSubsystem(const Class& InClass) const;
    template<typename T>
    T* GetSubsystem() const {
        return Cast<T>(GetSubsystem(T::StaticClass()));
    }

    void ResolvePendingKills();

    /** Every Node registered in this World */
    const Array<Node*>& GetAllNodes() const;

    CameraNode* GetMainCamera() const;
    void SetMainCamera(CameraNode* InCamera);

    Node* Spawn(const Class& InClass);
    template<typename T>
    T* Spawn() {
        return Spawn(T::StaticClass())->template As<T>();
    }

    SceneRootNode* Populate(Scene* InScene);

private:
    struct LocalUpdateChunk {
        Node* Root = nullptr;
        Array<Node*> Nodes;
    };

    void UnregisterNode(Node* node);
    void ReregisterNode(Node* node);

    void ResolveAllBeginPlayIssues();

    void WorldUpdate(float deltaTime);
    void HalfWorldUpdate(float InDeltaTime);
    void PrepareLocalUpdate();
    void LocalUpdate(float InDeltaTime);

private:
    Array<Node*> m_WorldNodes;
    Array<Node*> m_WorldUpdateNodes;

    Array<Node*> m_HalfWorldUpdateNodes[2];
    float m_HalfWorldElapsed[2] = { 0.0f, 0.0f };
    int32_t m_HalfWorldSide = 0;

    Array<Node*> m_LocalUpdateNodes;
    Array<LocalUpdateChunk> m_LocalUpdateChunks;

    /** Array of all nodes, that need to have BeginPlay() called. */
    Array<WeakObjectPtr<Node>> m_BeginPlayIssues;

    /** Array of all nodes, that will be killed at the end of the frame.
    *   Kill happens outside of Gameplay Context. */
    Array<Node*> m_PendingKills;

    WeakObjectPtr<CameraNode> m_MainCamera;

    Array<SharedObjectPtr<WorldSubsystem>> m_Subsystems;

    friend class Node;
};