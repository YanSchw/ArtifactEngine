#include "World.h"
#include "Node.h"
#include "Component.h"
#include "Core/Assert.h"

void World::Update(double InDeltatime) {
    ResolveAllBeginPlayIssues();

    // GetPhysicsWorld()->Update(deltaTime);

    // WorldUpdate + PrepareLocalUpdate
    /*m_WorldUpdateThread = std::thread(&World::WorldUpdate, this, deltaTime);
    m_PrepareLocalUpdateThread = std::thread(&World::PrepareLocalUpdate, this);

    m_WorldUpdateThread.join();
    m_PrepareLocalUpdateThread.join();*/
    WorldUpdate(InDeltatime);
    PrepareLocalUpdate();

    // LocalUpdate
    // UpdateRules::s_LocalUpdate = true;
    // for (LocalUpdateChunk& chunk : m_LocalUpdateChunks)
    // {
    //     chunk.m_Thread = std::thread(&World::LocalUpdate, this, deltaTime, &chunk);
    // }
    // for (LocalUpdateChunk& chunk : m_LocalUpdateChunks)
    // {
    //     chunk.m_Thread.join();
    // }
    // UpdateRules::s_LocalUpdate = false;

    ResolvePendingKills();
}

Node* World::Spawn(const Class& InClass) {
    AE_ASSERT(!InClass.IsSubclassOf(Component::StaticClass()), "A Component may never be spawned!");

    Node* node = Object::Create(InClass)->As<Node>();
    AE_ASSERT(node);

    // node->m_IsActorLayer = true;

    node->InitializeNode(*this);

    return node;
}

Array<Node*> World::GetAllNodes() const {
    return m_WorldNodes;
}

CameraNode* World::GetMainCamera() const {
    return m_MainCamera;
}

void World::SetMainCamera(CameraNode* InCamera) {
    m_MainCamera = InCamera;
}

void World::UnregisterNode(Node* node) {
    if (m_WorldNodes.Contains(node)) m_WorldNodes.Remove(node);
    if (node->IsUpdateFlagSet(UpdateFlag::WorldUpdate) && m_WorldUpdateNodes.Contains(node)) m_WorldUpdateNodes[m_WorldUpdateNodes.IndexOf(node)] = nullptr;
    if (node->IsUpdateFlagSet(UpdateFlag::LocalUpdate) && m_LocalUpdateNodes.Contains(node)) m_LocalUpdateNodes.Remove(node);
}
void World::ReregisterNode(Node* node) {
    if (node->IsUpdateFlagSet(UpdateFlag::WorldUpdate) && !m_WorldUpdateNodes.Contains(node)) m_WorldUpdateNodes.Add(node);
    if (node->IsUpdateFlagSet(UpdateFlag::LocalUpdate) && !m_LocalUpdateNodes.Contains(node)) m_LocalUpdateNodes.Add(node);
}

void World::ResolveAllBeginPlayIssues() {
    while (m_BeginPlayIssues.Size() > 0) {
        if (m_BeginPlayIssues[0] != nullptr) {
            if (!m_BeginPlayIssues[0]->WasBeginPlayCalled()) {
                m_BeginPlayIssues[0]->m_WasBeginPlayCalled = true;
                m_BeginPlayIssues[0]->BeginPlay();
            }

            // Push UpdateFlags
            ReregisterNode(m_BeginPlayIssues[0]);
        }
        m_BeginPlayIssues.RemoveAt(0);
    }
}

void World::ResolvePendingKills() {
    // Kill all Nodes in m_PendingKills
    for (Node* node : m_PendingKills) {
        node->UnInitializeNode(*this);
        delete node;
    }
    m_PendingKills.Clear();
}

void World::WorldUpdate(float deltaTime) {
    for (int64_t i = m_WorldUpdateNodes.Last(); i >= 0; i--) {
        if (m_WorldUpdateNodes[i]) {
            if (m_WorldUpdateNodes[i]->ShouldUpdateInCurrentContext()) {
                m_WorldUpdateNodes[i]->WorldUpdate(deltaTime);
            }
        } else {
            m_WorldUpdateNodes.RemoveAt(i);
        }
    }
}

void World::PrepareLocalUpdate() {
    
}