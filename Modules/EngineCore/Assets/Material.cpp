#include "Material.h"

#include "Assets/AssetManager.h"
#include "Assets/ShaderGraph.h"
#include "Assets/Texture2D.h"
#include "Rendering/Buffer.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/ShaderTemplate.h"

Material* Material::CreateEmpty(const String& InDirectory, const String& InName) {
    Material* asset = Cast<Material>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!asset) {
        return nullptr;
    }

    AssetManager::Get().SaveAsset(asset);
    return asset;
}

ShaderGraph* Material::GetBaseGraph() const {
    return m_BaseGraph.Get();
}

void Material::SetBaseGraph(ShaderGraph* InShaderGraph) {
    if (GetBaseGraph() == InShaderGraph) {
        return;
    }

    m_BaseGraph = InShaderGraph;
    m_InputNames.Clear();
    m_InputValues.Clear();
    m_TextureNames.Clear();
    m_TextureValues.Clear();

    if (InShaderGraph) {
        AssetManager::Get().LoadAsset(InShaderGraph);
    }
    RefreshPropertyBuffer();
}

Material* Material::GetParentMaterial() const {
    Material* base = GetBaseGraph();
    return base == this ? nullptr : base;
}

Shader* Material::GetShader() const {
    ShaderGraph* base = GetBaseGraph();
    return base ? base->GetCompiledShader() : nullptr;
}

const Array<ShaderGraphProperty>& Material::GetInputs() const {
    static const Array<ShaderGraphProperty> s_NoInputs;
    ShaderGraph* base = GetBaseGraph();
    return base ? base->GetDeclaredInputs() : s_NoInputs;
}

Array<MaterialTextureBinding> Material::GetTextureBindings() const {
    ShaderGraph* base = GetBaseGraph();
    if (!base) {
        return Array<MaterialTextureBinding>();
    }

    Array<MaterialTextureBinding> bindings = base->GetGraphTextureBindings();
    for (MaterialTextureBinding& binding : bindings) {
        if (Texture2D* texture = GetInputTexture(binding.Name)) {
            binding.Texture = texture;
        }
    }
    return bindings;
}

Vec4 Material::GetInputValue(const String& InName) const {
    const int32_t index = m_InputNames.IndexOf(InName);
    if (index >= 0 && index < m_InputValues.Size()) {
        return m_InputValues[index];
    }
    if (Material* parent = GetParentMaterial()) {
        return parent->GetInputValue(InName);
    }
    for (const ShaderGraphProperty& input : GetInputs()) {
        if (input.Name == InName) {
            return input.DefaultValue;
        }
    }
    return Vec4(0.0f);
}

void Material::SetInputValue(const String& InName, const Vec4& InValue) {
    const int32_t index = m_InputNames.IndexOf(InName);
    if (index >= 0 && index < m_InputValues.Size()) {
        m_InputValues[index] = InValue;
    } else {
        m_InputNames.Add(InName);
        m_InputValues.Add(InValue);
    }
    RefreshPropertyBuffer();
}

Texture2D* Material::GetInputTexture(const String& InName) const {
    const int32_t index = m_TextureNames.IndexOf(InName);
    if (index >= 0 && index < m_TextureValues.Size()) {
        return m_TextureValues[index].Get();
    }
    Material* parent = GetParentMaterial();
    return parent ? parent->GetInputTexture(InName) : nullptr;
}

void Material::SetInputTexture(const String& InName, Texture2D* InTexture) {
    const int32_t index = m_TextureNames.IndexOf(InName);
    if (index >= 0 && index < m_TextureValues.Size()) {
        m_TextureValues[index] = InTexture;
    } else {
        m_TextureNames.Add(InName);
        m_TextureValues.Add(WeakObjectPtr<Texture2D>(InTexture));
    }
    if (InTexture) {
        AssetManager::Get().LoadAsset(InTexture);
    }
}

bool Material::IsInputOverridden(const String& InName) const {
    return m_InputNames.Contains(InName) || m_TextureNames.Contains(InName);
}

void Material::ClearInputOverride(const String& InName) {
    const int32_t valueIndex = m_InputNames.IndexOf(InName);
    if (valueIndex >= 0 && valueIndex < m_InputValues.Size()) {
        m_InputNames.RemoveAt(valueIndex);
        m_InputValues.RemoveAt(valueIndex);
    }

    const int32_t textureIndex = m_TextureNames.IndexOf(InName);
    if (textureIndex >= 0 && textureIndex < m_TextureValues.Size()) {
        m_TextureNames.RemoveAt(textureIndex);
        m_TextureValues.RemoveAt(textureIndex);
    }
    RefreshPropertyBuffer();
}

void Material::RefreshPropertyBuffer() {
    ShaderGraph* base = GetBaseGraph();
    if (!RenderingAPI::GetInstance() || !base) {
        return;
    }

    const uint32_t blockSize = base->GetPropertyBlockSize();
    if (blockSize == 0) {
        m_PropertyBuffer = nullptr;
        m_BufferSize = 0;
        return;
    }

    if (!m_PropertyBuffer || m_BufferSize != blockSize) {
        m_BufferSize = blockSize;
        m_PropertyBuffer = UniformBuffer::Create(ShaderTemplate::MaterialUniformBinding, blockSize);
    }

    Array<byte> block;
    block.Resize(blockSize);
    memset(block.Data(), 0, block.Size());

    for (const ShaderGraphProperty& input : GetInputs()) {
        uint32_t offset = 0;
        if (input.IsTexture() || !base->FindInputOffset(input.Name, offset)) {
            continue;
        }
        const Vec4 value = GetInputValue(input.Name);
        memcpy(block.Data() + offset, &value, ShaderValue::GetSize(input.Type));
    }

    void* mapped = m_PropertyBuffer->MapData(block.Size(), 0);
    memcpy(mapped, block.Data(), block.Size());
    m_PropertyBuffer->UnmapData();
}

String Material::GetDisplayName() const {
    return DisplayNameFromPath(AssetManager::Get().GetAssetPath(GetId()));
}

bool Material::IsLoaded() const {
    return m_Loaded;
}

void Material::Load() {
    m_Loaded = true;

    Material* parent = GetParentMaterial();
    if (!parent) {
        return;
    }

    AssetManager::Get().LoadAsset(parent);
    for (const MaterialTextureBinding& binding : GetTextureBindings()) {
        AssetManager::Get().LoadAsset(binding.Texture);
    }
    RefreshPropertyBuffer();
}

void Material::Unload() {
    m_PropertyBuffer = nullptr;
    m_BufferSize = 0;
    m_Loaded = false;
}
