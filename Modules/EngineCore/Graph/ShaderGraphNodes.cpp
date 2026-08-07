#include "ShaderGraphNodes.h"

#include "Assets/Texture2D.h"

#include <cctype>
#include <format>

String ShaderGraphValueNode::GetSamplerName() const {
    return std::format("sg_Texture{0}", NodeId);
}

void ShaderGraphValueNode::ConstructPins() {
    SyncPins();
}

void ShaderGraphValueNode::SyncPins() {
    const bool isTexture = IsTexture();
    const bool matches = isTexture
        ? FindPin("RGBA", GraphPinDirection::Output) != nullptr
        : FindPin("Value", GraphPinDirection::Output) != nullptr;

    if (matches) {
        if (!isTexture) {
            FindPin("Value", GraphPinDirection::Output)->TypeName = ShaderValue::GetGlslType(ValueType);
        }
        return;
    }

    Pins.Clear();
    if (isTexture) {
        AddInput("UV", "vec2");
        AddOutput("RGBA", "vec4");
        AddOutput("R", "float");
        AddOutput("G", "float");
        AddOutput("B", "float");
        AddOutput("A", "float");
    } else {
        AddOutput("Value", ShaderValue::GetGlslType(ValueType));
    }
}

String ShaderGraphValueNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    if (!IsTexture()) {
        (void)InOutputPin;
        return IsInput() ? "Material." + InputName : ShaderValue::Literal(Value, ValueType);
    }

    if (!Texture.Get()) {
        InContext.AddError(std::format("texture node '{0}' has no texture assigned", GetTitle()));
        return InOutputPin.Name == "RGBA" ? "vec4(0.0)" : "0.0";
    }

    const String uv = InContext.HasInput(*this, "UV")
        ? InContext.ReadInput(*this, "UV", ShaderValueType::Vec2)
        : String("sg_UV");
    const String sample = std::format("texture({0}, {1})", GetSamplerName(), uv);
    if (InOutputPin.Name == "RGBA") {
        return sample;
    }
    return std::format("({0}).{1}", sample, (char)std::tolower(InOutputPin.Name[0]));
}

String ShaderGraphValueNode::GetTitle() const {
    if (IsInput()) {
        return std::format("Input {0}", InputName);
    }
    if (IsTexture()) {
        Texture2D* texture = Texture.Get();
        return texture ? std::format("Texture {0}", texture->GetDisplayName()) : "Texture";
    }
    return std::format("Constant ({0})", ShaderValue::GetGlslType(ValueType));
}

void ShaderGraphBinaryNode::ConstructPins() {
    AddInput("A", "vec4");
    AddInput("B", "vec4");
    AddOutput("Result", "vec4");
}

String ShaderGraphBinaryNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    (void)InOutputPin;

    const String a = InContext.ReadInput(*this, "A", ShaderValueType::Vec4);
    const String b = InContext.ReadInput(*this, "B", ShaderValueType::Vec4);

    switch (Operation) {
        case ShaderBinaryOp::Add:      return std::format("({0} + {1})", a, b);
        case ShaderBinaryOp::Subtract: return std::format("({0} - {1})", a, b);
        case ShaderBinaryOp::Multiply: return std::format("({0} * {1})", a, b);
        case ShaderBinaryOp::Divide:   return std::format("({0} / max({1}, vec4(1e-6)))", a, b);
        case ShaderBinaryOp::Minimum:  return std::format("min({0}, {1})", a, b);
        case ShaderBinaryOp::Maximum:  return std::format("max({0}, {1})", a, b);
        case ShaderBinaryOp::Power:    return std::format("pow(max({0}, vec4(0.0)), {1})", a, b);
        case ShaderBinaryOp::Step:     return std::format("step({0}, {1})", a, b);
    }
    return a;
}

String ShaderGraphBinaryNode::GetTitle() const {
    return EShaderBinaryOp::ConvertEnumToString(Operation);
}

void ShaderGraphUnaryNode::ConstructPins() {
    AddInput("Input", "vec4");
    AddOutput("Result", "vec4");
}

String ShaderGraphUnaryNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    (void)InOutputPin;

    const String value = InContext.ReadInput(*this, "Input", ShaderValueType::Vec4);

    switch (Operation) {
        case ShaderUnaryOp::OneMinus:  return std::format("(vec4(1.0) - {0})", value);
        case ShaderUnaryOp::Saturate:  return std::format("clamp({0}, vec4(0.0), vec4(1.0))", value);
        case ShaderUnaryOp::Negate:    return std::format("(-{0})", value);
        case ShaderUnaryOp::Absolute:  return std::format("abs({0})", value);
        case ShaderUnaryOp::Floor:     return std::format("floor({0})", value);
        case ShaderUnaryOp::Fraction:  return std::format("fract({0})", value);
        case ShaderUnaryOp::Sine:      return std::format("sin({0})", value);
        case ShaderUnaryOp::Cosine:    return std::format("cos({0})", value);
        case ShaderUnaryOp::Normalize: return std::format("vec4(normalize(({0}).xyz), ({0}).w)", value);
    }
    return value;
}

String ShaderGraphUnaryNode::GetTitle() const {
    return EShaderUnaryOp::ConvertEnumToString(Operation);
}

void ShaderGraphLerpNode::ConstructPins() {
    AddInput("A", "vec4");
    AddInput("B", "vec4");
    AddInput("Alpha", "float")->DefaultValue = Vec4(0.5f);
    AddOutput("Result", "vec4");
}

String ShaderGraphLerpNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    (void)InOutputPin;

    const String a = InContext.ReadInput(*this, "A", ShaderValueType::Vec4);
    const String b = InContext.ReadInput(*this, "B", ShaderValueType::Vec4);
    const String alpha = InContext.ReadInput(*this, "Alpha", ShaderValueType::Float);
    return std::format("mix({0}, {1}, {2})", a, b, alpha);
}

void ShaderGraphDotNode::ConstructPins() {
    AddInput("A", "vec4");
    AddInput("B", "vec4");
    AddOutput("Result", "float");
}

String ShaderGraphDotNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    (void)InOutputPin;

    const String a = InContext.ReadInput(*this, "A", ShaderValueType::Vec4);
    const String b = InContext.ReadInput(*this, "B", ShaderValueType::Vec4);
    return std::format("dot({0}, {1})", a, b);
}

void ShaderGraphInputNode::ConstructPins() {
    AddOutput("Value", "vec4");
    SyncPins();
}

static const char* VertexInputType(ShaderVertexInput InInput) {
    switch (InInput) {
        case ShaderVertexInput::UV:            return "vec2";
        case ShaderVertexInput::VertexColor:   return "vec4";
        case ShaderVertexInput::WorldPosition: return "vec3";
        case ShaderVertexInput::Time:          return "float";
    }
    return "vec4";
}

void ShaderGraphInputNode::SyncPins() {
    if (GraphPin* pin = FindPin("Value", GraphPinDirection::Output)) {
        pin->TypeName = VertexInputType(Input);
    }
}

String ShaderGraphInputNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    (void)InOutputPin;
    (void)InContext;

    switch (Input) {
        case ShaderVertexInput::UV:            return "sg_UV";
        case ShaderVertexInput::VertexColor:   return "sg_VertexColor";
        case ShaderVertexInput::WorldPosition: return "sg_WorldPosition";
        case ShaderVertexInput::Time:          return "sg_Time";
    }
    return "vec4(0.0)";
}

String ShaderGraphInputNode::GetTitle() const {
    return EShaderVertexInput::ConvertEnumToString(Input);
}

void ShaderGraphCombineNode::ConstructPins() {
    AddInput("X", "float");
    AddInput("Y", "float");
    AddInput("Z", "float");
    AddInput("W", "float")->DefaultValue = Vec4(1.0f);
    AddOutput("Result", "vec4");
}

String ShaderGraphCombineNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    (void)InOutputPin;

    return std::format("vec4({0}, {1}, {2}, {3})",
                       InContext.ReadInput(*this, "X", ShaderValueType::Float),
                       InContext.ReadInput(*this, "Y", ShaderValueType::Float),
                       InContext.ReadInput(*this, "Z", ShaderValueType::Float),
                       InContext.ReadInput(*this, "W", ShaderValueType::Float));
}

void ShaderGraphSplitNode::ConstructPins() {
    AddInput("Input", "vec4");
    AddOutput("X", "float");
    AddOutput("Y", "float");
    AddOutput("Z", "float");
    AddOutput("W", "float");
}

String ShaderGraphSplitNode::Emit(const GraphPin& InOutputPin, ShaderGraphContext& InContext) const {
    const String value = InContext.ReadInput(*this, "Input", ShaderValueType::Vec4);
    return std::format("({0}).{1}", value, (char)std::tolower(InOutputPin.Name[0]));
}

void ShaderGraphOutputNode::SyncPropertyPins(const Array<ShaderGraphProperty>& InProperties) {
    Array<SharedObjectPtr<GraphPin>> kept;

    for (const ShaderGraphProperty& property : InProperties) {
        GraphPin* existing = FindPin(property.Name, GraphPinDirection::Input);
        if (existing) {
            existing->TypeName = ShaderValue::GetGlslType(property.Type);
            for (const SharedObjectPtr<GraphPin>& pin : Pins) {
                if (pin.Get() == existing) {
                    kept.Add(pin);
                    break;
                }
            }
            continue;
        }

        GraphPin* pin = Object::Create<GraphPin>();
        pin->Name = property.Name;
        pin->TypeName = ShaderValue::GetGlslType(property.Type);
        pin->Direction = GraphPinDirection::Input;
        pin->DefaultValue = property.DefaultValue;
        kept.Add(SharedObjectPtr<GraphPin>(pin));
    }

    Pins = kept;
}
