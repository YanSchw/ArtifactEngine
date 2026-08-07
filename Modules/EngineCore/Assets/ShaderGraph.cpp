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

String ShaderGraph::GetDisplayName() const {
    return m_Graph && !m_Graph->GraphName.empty()
        ? m_Graph->GraphName
        : DisplayNameFromPath(AssetManager::Get().GetAssetPath(GetId()));
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
    m_Inputs = m_Template.GetProperties();
    m_InputErrors.Clear();

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

Vec4 ShaderGraph::GetInputValue(const String& InName) const {
    const int32_t index = m_InputNames.IndexOf(InName);
    if (index >= 0 && index < m_InputValues.Size()) {
        return m_InputValues[index];
    }
    for (const ShaderGraphProperty& input : m_Inputs) {
        if (input.Name == InName) {
            return input.DefaultValue;
        }
    }
    return Vec4(0.0f);
}

void ShaderGraph::SetInputValue(const String& InName, const Vec4& InValue) {
    const int32_t index = m_InputNames.IndexOf(InName);
    if (index >= 0 && index < m_InputValues.Size()) {
        m_InputValues[index] = InValue;
    } else {
        m_InputNames.Add(InName);
        m_InputValues.Add(InValue);
    }
    UploadPropertyBuffer();
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

Array<ShaderGraphTextureBinding> ShaderGraph::GetTextureBindings() const {
    Array<ShaderGraphTextureBinding> bindings;
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
    UploadPropertyBuffer();
    return true;
}

void ShaderGraph::UploadPropertyBuffer() {
    if (!RenderingAPI::GetInstance() || m_BlockSize == 0) {
        return;
    }

    if (!m_PropertyBuffer || m_BufferSize != m_BlockSize) {
        m_BufferSize = m_BlockSize;
        m_PropertyBuffer = UniformBuffer::Create(ShaderTemplate::MaterialUniformBinding, m_BlockSize);
    }

    Array<byte> block;
    block.Resize(m_BlockSize);
    memset(block.Data(), 0, block.Size());

    for (const ShaderGraphProperty& input : m_Inputs) {
        if (input.IsTexture()) {
            continue;
        }
        const Vec4 value = GetInputValue(input.Name);
        if (m_InputOffsets.ContainsKey(input.Name)) {
            memcpy(block.Data() + m_InputOffsets.At(input.Name), &value, ShaderValue::GetSize(input.Type));
        }
    }

    void* mapped = m_PropertyBuffer->MapData(block.Size(), 0);
    memcpy(mapped, block.Data(), block.Size());
    m_PropertyBuffer->UnmapData();
}

void ShaderGraph::Load() {
    EnsureTemplateLoaded();

    if (EngineConfig::IsPackagedBuild()) {
        SyncGraph();
        m_Shader = ShaderLibrary::CreateShader(GetShaderKey());
        UploadPropertyBuffer();
        return;
    }

    String error;
    if (!Recompile(error)) {
        AE_ERROR("ShaderGraph '{0}' failed to compile:\n{1}", GetDisplayName(), error);
    }
}

void ShaderGraph::Unload() {
    m_Shader = nullptr;
    m_PropertyBuffer = nullptr;
}
