#pragma once
#include "Graph/GraphNode.h"
#include "Rendering/ShaderGraphTypes.h"
#include "ShaderGraphNode.gen.h"

class ShaderGraphContext;

class ShaderGraphNode : public GraphNode {
public:
    ARTIFACT_CLASS();

    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const;
    virtual void SyncPins() { }

    ShaderValueType GetOutputType(const GraphPin& InOutputPin) const;

    virtual String GetCategory() const override { return "Shader"; }
    virtual Vec4 GetAccentColor() const override;
};

/** Resolves pin values into GLSL, hoisting shared subexpressions into temporaries and rejecting
 *  cycles. Temporaries are scoped to one property expansion, since sites can live in different
 *  stages. */
class ShaderGraphContext {
public:
    explicit ShaderGraphContext(const class NodeGraph& InGraph) : m_Graph(InGraph) { }

    String ReadInput(const GraphNode& InNode, const String& InPinName, ShaderValueType InExpectedType);
    bool HasInput(const GraphNode& InNode, const String& InPinName) const;
    String Hoist(const String& InExpression, ShaderValueType InType);
    void BeginProperty();

    const String& GetPrelude() const { return m_Prelude; }
    const Array<String>& GetErrors() const { return m_Errors; }
    void AddError(const String& InError) { m_Errors.Add(InError); }

private:
    bool FindUpstream(const GraphNode& InNode, const String& InPinName,
                      const GraphNode*& OutNode, const GraphPin*& OutPin) const;

    const class NodeGraph& m_Graph;
    String m_Prelude;
    Array<String> m_Errors;
    Array<uint64_t> m_Visiting;
    Map<String, String> m_Cache;
    uint32_t m_NextTemp = 0;
};
