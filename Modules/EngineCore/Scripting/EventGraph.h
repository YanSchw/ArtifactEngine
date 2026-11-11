#pragma once
#include "Graph/NodeGraph.h"
#include "EventGraph.gen.h"

class DelegateEventNode;

class EventGraph : public NodeGraph {
public:
    ARTIFACT_CLASS();

    EventGraph();

    PROPERTY()
    String SelfClass = "Node";

    inline static const String ExecType = "Exec";

    static String PinTypeOf(Property* InProperty);
    static String ObjectPinType(const Class& InClass) { return "Object:" + InClass.Name; }
    static String PrettyName(const String& InName);

    void RefreshNodes();

    DelegateEventNode* FindDelegateEvent(const String& InNodeName, const String& InDelegateName) const;
    DelegateEventNode* AddDelegateEvent(const String& InNodeName, const String& InDelegateName, const Class& InOwnerClass);
    Vec2 FindFreeSpot() const;

    virtual bool ArePinTypesCompatible(const String& InOutputType, const String& InInputType) const override;
    virtual bool AllowsNodeClass(const Class& InNodeClass) const override;
    virtual bool AllowsMultipleConnections(const GraphPin& InPin) const override;
    virtual void CollectNodeTemplates(Array<GraphNodeTemplate>& OutTemplates, Object* InContext) const override;
};
