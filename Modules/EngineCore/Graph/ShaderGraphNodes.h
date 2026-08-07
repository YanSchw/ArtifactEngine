#pragma once
#include "ShaderGraphNode.h"
#include "ShaderGraphNodes.gen.h"

class Texture2D;

ARTIFACT_ENUM();
enum class ShaderBinaryOp : uint32_t {
    Add,
    Subtract,
    Multiply,
    Divide,
    Minimum,
    Maximum,
    Power,
    Step
};

ARTIFACT_ENUM();
enum class ShaderUnaryOp : uint32_t {
    OneMinus,
    Saturate,
    Negate,
    Absolute,
    Floor,
    Fraction,
    Sine,
    Cosine,
    Normalize
};

ARTIFACT_ENUM();
enum class ShaderVertexInput : uint32_t {
    UV,
    VertexColor,
    WorldPosition,
    Time
};

/** A value the graph feeds in: a literal, or a Texture2D asset. Marking it an input publishes it
 *  under a name so material instances can override it. */
class ShaderGraphValueNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    ShaderValueType ValueType = ShaderValueType::Float;

    PROPERTY()
    Vec4 Value = Vec4(1.0f);

    PROPERTY()
    WeakObjectPtr<Texture2D> Texture;

    PROPERTY()
    bool ExposeAsInput = false;

    PROPERTY()
    String InputName;

    bool IsTexture() const { return ValueType == ShaderValueType::Texture2D; }
    bool IsInput() const { return ExposeAsInput && !InputName.empty(); }
    String GetSamplerName() const;

    virtual void ConstructPins() override;
    virtual void SyncPins() override;
    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const override;
    virtual String GetTitle() const override;
    virtual String GetCategory() const override { return "Constants"; }
};

class ShaderGraphBinaryNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    ShaderBinaryOp Operation = ShaderBinaryOp::Add;

    virtual void ConstructPins() override;
    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const override;
    virtual String GetTitle() const override;
    virtual String GetCategory() const override { return "Math"; }
};

class ShaderGraphUnaryNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    ShaderUnaryOp Operation = ShaderUnaryOp::OneMinus;

    virtual void ConstructPins() override;
    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const override;
    virtual String GetTitle() const override;
    virtual String GetCategory() const override { return "Math"; }
};

class ShaderGraphLerpNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const override;
    virtual String GetTitle() const override { return "Lerp"; }
    virtual String GetCategory() const override { return "Math"; }
};

class ShaderGraphDotNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const override;
    virtual String GetTitle() const override { return "Dot"; }
    virtual String GetCategory() const override { return "Math"; }
};

class ShaderGraphInputNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    ShaderVertexInput Input = ShaderVertexInput::UV;

    virtual void ConstructPins() override;
    virtual void SyncPins() override;
    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const override;
    virtual String GetTitle() const override;
    virtual String GetCategory() const override { return "Input"; }
};

class ShaderGraphCombineNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const override;
    virtual String GetTitle() const override { return "Combine"; }
    virtual String GetCategory() const override { return "Vector"; }
};

class ShaderGraphSplitNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    virtual void ConstructPins() override;
    virtual String Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const override;
    virtual String GetTitle() const override { return "Split"; }
    virtual String GetCategory() const override { return "Vector"; }
};

class ShaderGraphOutputNode : public ShaderGraphNode {
public:
    ARTIFACT_CLASS();

    virtual String GetTitle() const override { return "Output"; }
    virtual String GetCategory() const override { return "Output"; }
    virtual bool IsUserCreatable() const override { return false; }

    void SyncPropertyPins(const Array<ShaderGraphProperty>& InProperties);
};
