#include "ShaderGraph.h"

#include "Assets/AssetManager.h"
#include "Assets/Texture2D.h"
#include "Core/EngineConfig.h"
#include "Graph/ShaderGraphNodes.h"
#include "Graph/ShaderNodeGraph.h"
#include "Rendering/Buffer.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"

#include <format>

ShaderGraph* ShaderGraph::CreateEmpty(const String& InDirectory, const String& InName) {
    ShaderGraph* asset = Cast<ShaderGraph>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!asset) {
        return nullptr;
    }

    asset->m_Graph = Object::Create<ShaderNodeGraph>();
    asset->m_Graph->GraphName = InName;
    asset->EnsureTemplateLoaded();
    asset->SyncGraph();

    AssetManager::Get().SaveAsset(asset);
    return asset;
}

bool ShaderGraph::IsLoaded() const {
    return m_Shader.Get() != nullptr;
}

bool ShaderGraph::EnsureTemplateLoaded() {
    if (m_TemplateLoaded) {
        return true;
    }

    // Left unloaded on failure so a fixed template is picked up on the next attempt, and so a
    // half-parsed template never reaches the graph.
    if (!ShaderTemplate::Parse(m_TemplatePath, m_Template, m_TemplateError)) {
        return false;
    }

    m_TemplateLoaded = true;
    m_TemplateError.clear();
    return true;
}

void ShaderGraph::SetTemplatePath(const String& InPath) {
    if (m_TemplatePath == InPath) {
        return;
    }
    m_TemplatePath = InPath;
    m_TemplateLoaded = false;
    m_Template = ShaderTemplate();
    EnsureTemplateLoaded();
    SyncGraph();
}

ShaderGraphOutputNode* ShaderGraph::FindOutputNode() const {
    if (!m_Graph) {
        return nullptr;
    }
    for (const SharedObjectPtr<GraphNode>& node : m_Graph->Nodes) {
        if (ShaderGraphOutputNode* output = Cast<ShaderGraphOutputNode>(node.Get())) {
            return output;
        }
    }
    return nullptr;
}

void ShaderGraph::SyncGraph() {
    if (!m_Graph) {
        m_Graph = Object::Create<ShaderNodeGraph>();
    } else if (!Cast<ShaderNodeGraph>(m_Graph.Get())) {
        SharedObjectPtr<ShaderNodeGraph> upgraded = Object::Create<ShaderNodeGraph>();
        upgraded->GraphName = m_Graph->GraphName;
        upgraded->NextNodeId = m_Graph->NextNodeId;
        upgraded->Nodes = m_Graph->Nodes;
        upgraded->Connections = m_Graph->Connections;
        m_Graph = upgraded;
    }

    for (const SharedObjectPtr<GraphNode>& node : m_Graph->Nodes) {
        if (ShaderGraphNode* shaderNode = Cast<ShaderGraphNode>(node.Get())) {
            shaderNode->SyncPins();
        }
    }

    // Rewriting the output pins against a template that failed to parse would drop every pin and
    // then every wire into it, so the graph is left untouched until the template is valid again.
    if (!m_TemplateLoaded) {
        return;
    }

    ShaderGraphOutputNode* output = FindOutputNode();
    if (!output) {
        output = m_Graph->CreateNode<ShaderGraphOutputNode>(Vec2(320.0f, 40.0f));
    }
    output->SyncPropertyPins(m_Template.GetProperties());

    m_Graph->PruneInvalidConnections();
    BuildInputs();
}

void ShaderGraph::BuildInputs() {
    m_Inputs.Clear();
    m_InputErrors.Clear();

    // A wired property is the graph's answer, leaving nothing for a material to override.
    ShaderGraphOutputNode* output = FindOutputNode();
    for (const ShaderGraphProperty& property : m_Template.GetProperties()) {
        GraphPin* pin = output ? output->FindPin(property.Name, GraphPinDirection::Input) : nullptr;
        if (!pin || !m_Graph->IsPinConnected(*output, *pin)) {
            m_Inputs.Add(property);
        }
    }

    for (const SharedObjectPtr<GraphNode>& node : m_Graph->Nodes) {
        ShaderGraphValueNode* value = Cast<ShaderGraphValueNode>(node.Get());
        if (!value || !value->IsInput()) {
            continue;
        }

        const ShaderGraphProperty* existing = nullptr;
        for (const ShaderGraphProperty& input : m_Inputs) {
            if (input.Name == value->InputName) {
                existing = &input;
                break;
            }
        }

        if (existing) {
            if (existing->Type != value->ValueType) {
                m_InputErrors.Add(std::format("input '{0}' is declared as both {1} and {2}",
                                              value->InputName,
                                              ShaderValue::GetGlslType(existing->Type),
                                              ShaderValue::GetGlslType(value->ValueType)));
            }
            continue;
        }

        ShaderGraphProperty input;
        input.Name = value->InputName;
        input.Identifier = value->InputName;
        input.Type = value->ValueType;
        input.DefaultValue = value->Value;
        m_Inputs.Add(input);
    }

    BuildInputLayout();

    for (int32_t i = m_InputNames.Size() - 1; i >= 0; i--) {
        bool live = false;
        for (const ShaderGraphProperty& input : m_Inputs) {
            live = live || input.Name == m_InputNames[i];
        }
        if (!live) {
            m_InputNames.RemoveAt(i);
            if (i < m_InputValues.Size()) {
                m_InputValues.RemoveAt(i);
            }
        }
    }
}

void ShaderGraph::BuildInputLayout() {
    m_InputOffsets.Clear();
    uint32_t offset = 0;
    for (const ShaderGraphProperty& input : m_Inputs) {
        if (input.IsTexture()) {
            continue;
        }
        const uint32_t alignment = ShaderValue::GetAlignment(input.Type);
        offset = (offset + alignment - 1) / alignment * alignment;
        m_InputOffsets[input.Name] = offset;
        offset += ShaderValue::GetSize(input.Type);
    }
    m_BlockSize = (offset + 15) / 16 * 16;
}

bool ShaderGraph::FindInputOffset(const String& InName, uint32_t& OutOffset) const {
    if (!m_InputOffsets.ContainsKey(InName)) {
        return false;
    }
    OutOffset = m_InputOffsets.At(InName);
    return true;
}

String ShaderGraph::GetStateValue(const String& InState) const {
    const int32_t index = m_StateNames.IndexOf(InState);
    if (index >= 0 && index < m_StateValues.Size()
        && m_Template.GetStateOptions(InState).Contains(m_StateValues[index])) {
        return m_StateValues[index];
    }
    return m_Template.GetDefaultStateValue(InState);
}

void ShaderGraph::SetStateValue(const String& InState, const String& InValue) {
    const int32_t index = m_StateNames.IndexOf(InState);
    if (index >= 0 && index < m_StateValues.Size()) {
        m_StateValues[index] = InValue;
    } else {
        m_StateNames.Add(InState);
        m_StateValues.Add(InValue);
    }
}

Array<MaterialTextureBinding> ShaderGraph::GetGraphTextureBindings() const {
    Array<MaterialTextureBinding> bindings;
    if (!m_Graph) {
        return bindings;
    }

    for (const SharedObjectPtr<GraphNode>& node : m_Graph->Nodes) {
        ShaderGraphValueNode* value = Cast<ShaderGraphValueNode>(node.Get());
        if (!value || !value->IsTexture() || !value->Texture.Get()) {
            continue;
        }
        bindings.Add({ ShaderTemplate::MaterialTextureBase + (uint32_t)bindings.Size(),
                       value->IsInput() ? value->InputName : String(),
                       value->Texture.Get() });
    }
    return bindings;
}

String ShaderGraph::BuildDeclarations() const {
    String declarations;

    if (m_BlockSize > 0) {
        declarations = std::format("layout(binding = {0}, std140) uniform MaterialBlock {{\n",
                                   ShaderTemplate::MaterialUniformBinding);
        for (const ShaderGraphProperty& input : m_Inputs) {
            if (!input.IsTexture()) {
                declarations += std::format("    {0} {1};\n", ShaderValue::GetGlslType(input.Type), input.Name);
            }
        }
        declarations += "} Material;\n";
    }

    uint32_t binding = ShaderTemplate::MaterialTextureBase;
    for (const SharedObjectPtr<GraphNode>& node : m_Graph->Nodes) {
        ShaderGraphValueNode* value = Cast<ShaderGraphValueNode>(node.Get());
        if (!value || !value->IsTexture() || !value->Texture.Get()) {
            continue;
        }
        declarations += std::format("layout(binding = {0}) uniform sampler2D {1};\n",
                                    binding++, value->GetSamplerName());
    }

    return declarations;
}

String ShaderGraph::GenerateSource(String& OutError) const {
    if (!m_Graph) {
        OutError = "shader graph has no graph document";
        return "";
    }
    if (!m_TemplateLoaded) {
        OutError = m_TemplateError;
        return "";
    }
    if (!m_InputErrors.IsEmpty()) {
        OutError = "";
        for (const String& error : m_InputErrors) {
            OutError += error + "\n";
        }
        return "";
    }

    ShaderGraphContext context(*m_Graph);
    Map<String, ShaderPropertyCode> code;

    if (ShaderGraphOutputNode* output = FindOutputNode()) {
        for (const ShaderGraphProperty& property : m_Template.GetProperties()) {
            GraphPin* pin = output->FindPin(property.Name, GraphPinDirection::Input);
            if (!pin || !m_Graph->IsPinConnected(*output, *pin)) {
                continue;
            }

            context.BeginProperty();
            ShaderPropertyCode propertyCode;
            propertyCode.Expression = context.ReadInput(*output, property.Name, property.Type);
            propertyCode.Prelude = context.GetPrelude();
            code[property.Name] = propertyCode;
        }
    }

    if (!context.GetErrors().IsEmpty()) {
        OutError = "";
        for (const String& error : context.GetErrors()) {
            OutError += error + "\n";
        }
        return "";
    }

    Map<String, String> states;
    for (const String& state : m_Template.GetStateNames()) {
        states[state] = GetStateValue(state);
    }

    return m_Template.Expand(code, states, BuildDeclarations());
}

bool ShaderGraph::RegisterGeneratedSource() {
    EnsureTemplateLoaded();
    SyncGraph();

    String error;
    const String source = GenerateSource(error);
    if (source.empty()) {
        AE_ERROR("ShaderGraph '{0}' could not generate GLSL: {1}", GetDisplayName(), error);
        return false;
    }

    ShaderLibrary::RegisterSource(GetShaderKey(), source);
    return true;
}

bool ShaderGraph::Recompile(String& OutError) {
    if (!EnsureTemplateLoaded()) {
        OutError = m_TemplateError;
        return false;
    }
    SyncGraph();

    const String source = GenerateSource(OutError);
    if (source.empty()) {
        return false;
    }

    ShaderLibrary::RegisterSource(GetShaderKey(), source);
    if (!ShaderLibrary::Reload(GetShaderKey(), OutError)) {
        return false;
    }

    m_Shader = ShaderLibrary::Find(GetShaderKey());
    RefreshPropertyBuffer();
    return true;
}

void ShaderGraph::Load() {
    Material::Load();

    if (EngineConfig::IsPackagedBuild()) {
        BuildInputLayout();
        m_Shader = ShaderLibrary::CreateShader(GetShaderKey());
        RefreshPropertyBuffer();
        return;
    }

    EnsureTemplateLoaded();

    String error;
    if (!Recompile(error)) {
        AE_ERROR("ShaderGraph '{0}' failed to compile:\n{1}", GetDisplayName(), error);
    }
}

void ShaderGraph::Cook(ChunkedBinary& OutChunkedBinary) {
    EnsureTemplateLoaded();
    SyncGraph();
    Super::Cook(OutChunkedBinary);
}

void ShaderGraph::Unload() {
    m_Shader = nullptr;
    Material::Unload();
}
