#pragma once
#include "Graph/NodeGraph.h"
#include "EventGraph.gen.h"

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

    virtual bool ArePinTypesCompatible(const String& InOutputType, const String& InInputType) const override;
    virtual bool AllowsNodeClass(const Class& InNodeClass) const override;
    virtual bool AllowsMultipleConnections(const GraphPin& InPin) const override;
    virtual void CollectNodeTemplates(Array<GraphNodeTemplate>& OutTemplates, Object* InContext) const override;
};
