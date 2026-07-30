#include "World.h"
#include "Node.h"
#include "Component.h"
#include "Common/Map.h"
#include "Core/Assert.h"
#include "Core/MainThread.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

class ChunkedTaskPool {
public:
    static ChunkedTaskPool& Get() {
        static ChunkedTaskPool pool;
        return pool;
    }

    void Run(int32_t InCount, const std::function<void(int32_t)>& InTask) {
        if (InCount <= 0) {
            return;
        }
        if (InCount == 1 || m_Workers.IsEmpty()) {
            InTask(0);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Task = &InTask;
            m_NextIndex = 0;
            m_Count = InCount;
            m_Unfinished = InCount;
        }
        m_WorkAvailable.notify_all();

        TakeTasks();

        std::unique_lock<std::mutex> lock(m_Mutex);
        m_WorkFinished.wait(lock, [this] { return m_Unfinished == 0; });
        m_Task = nullptr;
    }

private:
    ChunkedTaskPool() {
        const uint32_t cores = std::thread::hardware_concurrency();
        const int32_t workers = (int32_t)(cores > 1 ? cores - 1 : 0);  // the calling thread is the last one
        for (int32_t i = 0; i < workers; i++) {
            m_Workers.Emplace([this] { WorkerLoop(); });
        }
    }

    ~ChunkedTaskPool() {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Quit = true;
        }
        m_WorkAvailable.notify_all();
        for (std::thread& worker : m_Workers) {
            worker.join();
        }
    }

    void WorkerLoop() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(m_Mutex);
                m_WorkAvailable.wait(lock, [this] { return m_Quit || (m_Task && m_NextIndex < m_Count); });
                if (m_Quit) {
                    return;
                }
            }
            TakeTasks();
        }
    }

    void TakeTasks() {
        while (true) {
            const std::function<void(int32_t)>* task = nullptr;
            int32_t index = 0;
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                if (!m_Task || m_NextIndex >= m_Count) {
                    return;
                }
                task = m_Task;
                index = m_NextIndex++;
            }

            (*task)(index);

            std::lock_guard<std::mutex> lock(m_Mutex);
            if (--m_Unfinished == 0) {
                m_WorkFinished.notify_all();
            }
        }
    }

    Array<std::thread> m_Workers;
    std::mutex m_Mutex;
    std::condition_variable m_WorkAvailable;
    std::condition_variable m_WorkFinished;
    const std::function<void(int32_t)>* m_Task = nullptr;
    int32_t m_NextIndex = 0;
    int32_t m_Count = 0;
    int32_t m_Unfinished = 0;
    bool m_Quit = false;
};

static int32_t LiveCount(const Array<Node*>& InNodes) {
    int32_t count = 0;
    for (Node* node : InNodes) {
        if (node) {
            count++;
        }
    }
    return count;
}

World::~World() {
    for (Node* node : m_WorldNodes) {
        if (!node->IsPendingKill()) {
            node->Destroy();
        }
    }
    ResolvePendingKills();
}

void World::Update(double InDeltatime) {
    AE_ASSERT_MAIN_THREAD("World::Update");

    ResolveAllBeginPlayIssues();

    // GetPhysicsWorld()->Update(deltaTime);

    WorldUpdate((float)InDeltatime);
    HalfWorldUpdate((float)InDeltatime);
    LocalUpdate((float)InDeltatime);

    ResolvePendingKills();
}

Node* World::Spawn(const Class& InClass) {
    AE_ASSERT_MAIN_THREAD("World::Spawn");
    AE_ASSERT(!InClass.IsSubclassOf(Component::StaticClass()), "A Component may never be spawned!");

    Node* node = Object::Create(InClass)->As<Node>();
    AE_ASSERT(node);

    // node->m_IsActorLayer = true;

    node->InitializeNode(*this);

    return node;
}

const Array<Node*>& World::GetAllNodes() const {
    return m_WorldNodes;
}

CameraNode* World::GetMainCamera() const {
    return m_MainCamera;
}

void World::SetMainCamera(CameraNode* InCamera) {
    AE_ASSERT_MAIN_THREAD("World::SetMainCamera");
    m_MainCamera = InCamera;
}

void World::UnregisterNode(Node* node) {
    if (m_WorldNodes.Contains(node)) m_WorldNodes.Remove(node);
    if (node->IsUpdateFlagSet(UpdateFlag::WorldUpdate) && m_WorldUpdateNodes.Contains(node)) m_WorldUpdateNodes[m_WorldUpdateNodes.IndexOf(node)] = nullptr;
    if (node->IsUpdateFlagSet(UpdateFlag::LocalUpdate) && m_LocalUpdateNodes.Contains(node)) m_LocalUpdateNodes.Remove(node);
    if (node->IsUpdateFlagSet(UpdateFlag::HalfWorldUpdate)) {
        for (Array<Node*>& half : m_HalfWorldUpdateNodes) {
            const int32_t index = half.IndexOf(node);
            if (index >= 0) half[index] = nullptr;  // the next pass over this half drops the hole
        }
    }
}
void World::ReregisterNode(Node* node) {
    if (node->IsUpdateFlagSet(UpdateFlag::WorldUpdate) && !m_WorldUpdateNodes.Contains(node)) m_WorldUpdateNodes.Add(node);
    if (node->IsUpdateFlagSet(UpdateFlag::LocalUpdate) && !m_LocalUpdateNodes.Contains(node)) m_LocalUpdateNodes.Add(node);
    if (node->IsUpdateFlagSet(UpdateFlag::HalfWorldUpdate)
        && !m_HalfWorldUpdateNodes[0].Contains(node) && !m_HalfWorldUpdateNodes[1].Contains(node)) {
        const bool second = LiveCount(m_HalfWorldUpdateNodes[1]) < LiveCount(m_HalfWorldUpdateNodes[0]);
        m_HalfWorldUpdateNodes[second ? 1 : 0].Add(node);
    }
}

void World::ResolveAllBeginPlayIssues() {
    while (m_BeginPlayIssues.Size() > 0) {
        Node* node = m_BeginPlayIssues[0].Get();
        m_BeginPlayIssues.RemoveAt(0);  // BeginPlay may spawn, which appends to this queue

        // Destroy() skipped its EndPlay because play never began; beginning it now unbalances that.
        if (!node || node->IsPendingKill()) {
            continue;
        }

        if (!node->WasBeginPlayCalled()) {
            node->m_WasBeginPlayCalled = true;
            node->BeginPlay();
        }

        // Push UpdateFlags
        ReregisterNode(node);
    }
}

void World::ResolvePendingKills() {
    AE_ASSERT_MAIN_THREAD("World::ResolvePendingKills");

    while (!m_PendingKills.IsEmpty()) {
        Node* node = m_PendingKills[0];
        m_PendingKills.RemoveAt(0);
        node->UnInitializeNode(*this);
        delete node;
    }
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

void World::HalfWorldUpdate(float InDeltaTime) {
    m_HalfWorldElapsed[0] += InDeltaTime;
    m_HalfWorldElapsed[1] += InDeltaTime;

    const int32_t side = m_HalfWorldSide;
    const float elapsed = m_HalfWorldElapsed[side];
    m_HalfWorldElapsed[side] = 0.0f;
    m_HalfWorldSide = 1 - side;

    Array<Node*>& half = m_HalfWorldUpdateNodes[side];
    for (int64_t i = half.Last(); i >= 0; i--) {
        if (!half[(int32_t)i]) {
            half.RemoveAt((int32_t)i);
        } else if (half[(int32_t)i]->ShouldUpdateInCurrentContext()) {
            half[(int32_t)i]->HalfWorldUpdate(elapsed);
        }
    }
}

void World::PrepareLocalUpdate() {
    m_LocalUpdateChunks.Clear();

    Map<Node*, int32_t> chunkOfRoot;
    for (Node* node : m_LocalUpdateNodes) {
        if (!node || !node->ShouldUpdateInCurrentContext()) {
            continue;
        }

        Node* root = node->GetRootNode();
        if (!chunkOfRoot.ContainsKey(root)) {
            chunkOfRoot[root] = m_LocalUpdateChunks.Size();
            LocalUpdateChunk chunk;
            chunk.Root = root;
            m_LocalUpdateChunks.Add(std::move(chunk));
        }
        m_LocalUpdateChunks[chunkOfRoot.At(root)].Nodes.Add(node);
    }
}

void World::LocalUpdate(float InDeltaTime) {
    PrepareLocalUpdate();
    if (m_LocalUpdateChunks.IsEmpty()) {
        return;
    }

    ChunkedTaskPool::Get().Run(m_LocalUpdateChunks.Size(), [this, InDeltaTime](int32_t InChunk) {
        const MainThread::LocalUpdateScope scope;
        for (Node* node : m_LocalUpdateChunks[InChunk].Nodes) {
            node->LocalUpdate(InDeltaTime);
        }
    });
}