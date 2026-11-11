#pragma once
#include "Asset.h"
#include "Common/Array.h"
#include "NodeAsset.gen.h"

class Node;
class NodeRecord;
class EventGraph;

class NodeAsset : public Asset {
public:
    ARTIFACT_CLASS();
    virtual ~NodeAsset() = default;

    NodeRecord* GetRoot() const;
    void SetRoot(const SharedObjectPtr<NodeRecord>& InRoot);
    void CaptureFrom(Node& InNode);

    Class GetRootClass() const;
    EventGraph* GetEventGraph() const;
    EventGraph* GetOrCreateEventGraph();
    void SetEventGraph(EventGraph* InGraph);
    Array<AssetStreamHandle> GetReferencedAssetHandles() const { return m_ReferencedAssets; }

    virtual String SerializeToJson() const override;
    virtual void DeserializeFromJson(const String& InJson) override;
    virtual bool IsLoaded() const override;

protected:
    virtual void Load() override;
    virtual void Unload() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

    void AcquireReferencedAssets();

    SharedObjectPtr<NodeRecord> m_Root;

    PROPERTY()
    SharedObjectPtr<EventGraph> m_EventGraph;

    Array<AssetStreamHandle> m_ReferencedAssets;
    bool m_Loaded = false;
};
