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

    FUNCTION(Pure)
    Class GetRootClass() const;
    EventGraph* GetEventGraph() const;
    EventGraph* GetOrCreateEventGraph();
    void SetEventGraph(EventGraph* InGraph);

    virtual String SerializeToJson() const override;
    virtual void DeserializeFromJson(const String& InJson) override;
    virtual bool IsLoaded() const override;

protected:
    virtual void Load() override;
    virtual void Unload() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;
    virtual void CollectDependencies(Array<Asset*>& OutDependencies) const override;

    SharedObjectPtr<NodeRecord> m_Root;

    PROPERTY()
    SharedObjectPtr<EventGraph> m_EventGraph;

    bool m_Loaded = false;
};
