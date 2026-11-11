#include "NodeAsset.h"
#include "NodeRecord.h"
#include "AssetManager.h"
#include "GameFramework/Node.h"
#include "Scripting/EventGraph.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Json.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"

static constexpr uint32_t s_NodeTreeChunk = 1;

NodeRecord* NodeAsset::GetRoot() const {
    return m_Root.Get();
}

void NodeAsset::SetRoot(const SharedObjectPtr<NodeRecord>& InRoot) {
    m_Root = InRoot;
}

void NodeAsset::CaptureFrom(Node& InNode) {
    m_Root = NodeRecord::Capture(InNode);
    if (m_Root.Get()) {
        m_Root->Name.clear();
    }
}

Class NodeAsset::GetRootClass() const {
    return m_Root.Get() ? Class(m_Root->ClassName) : Class::None;
}

EventGraph* NodeAsset::GetEventGraph() const {
    return m_EventGraph.Get();
}

EventGraph* NodeAsset::GetOrCreateEventGraph() {
    if (!m_EventGraph.Get()) {
        m_EventGraph = Object::Create<EventGraph>();
    }
    return m_EventGraph.Get();
}

void NodeAsset::SetEventGraph(EventGraph* InGraph) {
    m_EventGraph = InGraph;
}

String NodeAsset::SerializeToJson() const {
    nlohmann::json json = nlohmann::json::parse(JsonSerializer::SerializeObject(this));
    json["AssetClass"] = GetClass().Name;
    json["Root"] = m_Root.Get() ? m_Root->ToJson() : nlohmann::json();
    return json.dump(4);
}

void NodeAsset::DeserializeFromJson(const String& InJson) {
    JsonSerializer::DeserializeObject(this, InJson);

    const nlohmann::json json = nlohmann::json::parse(InJson, nullptr, false);
    if (json.is_discarded() || !json.contains("Root")) {
        return;
    }
    m_Root = NodeRecord::FromJson(json["Root"]);
    if (m_Root.Get()) {
        m_Root->Name.clear();
    }
}

bool NodeAsset::IsLoaded() const {
    return m_Loaded;
}

void NodeAsset::Load() {
    m_Loaded = true;

    if (EngineConfig::IsPackagedBuild()) {
        SharedObjectPtr<ChunkedBinary> binary = GetChunkedBinary();
        if (binary && binary->HasChunk(s_NodeTreeChunk)) {
            ChunkReader reader = binary->GetChunk(s_NodeTreeChunk);
            std::vector<uint8_t> bytes(reader.GetRemaining());
            if (!bytes.empty()) {
                reader.ReadBytes(bytes.data(), bytes.size());
            }
            m_Root = NodeRecord::FromBinary(bytes);
        }
    }

    AcquireReferencedAssets();
}

void NodeAsset::Unload() {
    m_ReferencedAssets.Clear();
    m_Loaded = false;
    if (EngineConfig::IsPackagedBuild()) {
        m_Root = nullptr;
    }
}

void NodeAsset::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    if (!m_Root.Get()) {
        return;
    }

    const std::vector<uint8_t> bytes = m_Root->ToBinary();
    ChunkWriter writer;
    writer.WriteBytes(bytes.data(), bytes.size());
    OutChunkedBinary.AddChunk(s_NodeTreeChunk, writer);
}

void NodeAsset::AcquireReferencedAssets() {
    m_ReferencedAssets.Clear();
    if (!m_Root.Get()) {
        return;
    }

    Array<UUID> ids;
    m_Root->CollectAssetReferences(ids);

    for (const UUID& id : ids) {
        if (id == GetId()) {
            continue;
        }
        if (Asset* referenced = AssetManager::Get().GetAsset(id)) {
            m_ReferencedAssets.Add(referenced->GetStreamHandle());
        } else {
            AE_WARN("Asset '{0}' references missing asset {1}", GetDisplayName(), id.ToString());
        }
    }
}
