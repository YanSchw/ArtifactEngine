#pragma once
#include "Asset.h"
#include "Object/Pointer.h"
#include "Rendering/ShaderGraphTypes.h"
#include "Material.gen.h"

class Shader;
class ShaderGraph;
class Texture2D;
class UniformBuffer;

struct MaterialTextureBinding {
    uint32_t Binding = 0;
    String Name;
    Texture2D* Texture = nullptr;
};

class Material : public Asset {
public:
    ARTIFACT_CLASS();

    virtual ~Material() = default;

    static Material* CreateEmpty(const String& InDirectory, const String& InName);
    /** The engine's default shader graph, drawn wherever no material is assigned. */
    static Material* GetDefault();

    FUNCTION(Pure)
    virtual ShaderGraph* GetBaseGraph() const;
    void SetBaseGraph(ShaderGraph* InShaderGraph);

    /** The material whose values are inherited where this one holds no override. */
    FUNCTION(Pure)
    Material* GetParentMaterial() const;

    Shader* GetShader() const;
    UniformBuffer* GetPropertyBuffer() const { return m_PropertyBuffer.Get(); }
    Array<MaterialTextureBinding> GetTextureBindings() const;

    bool IsReadyToRender();

    const Array<ShaderGraphProperty>& GetInputs() const;

    FUNCTION(Pure)
    Vec4 GetInputValue(const String& InName) const;
    FUNCTION()
    void SetInputValue(const String& InName, const Vec4& InValue);

    FUNCTION(Pure)
    Texture2D* GetInputTexture(const String& InName) const;
    FUNCTION()
    void SetInputTexture(const String& InName, Texture2D* InTexture);

    FUNCTION(Pure)
    bool IsInputOverridden(const String& InName) const;
    FUNCTION()
    void ClearInputOverride(const String& InName);

    void RefreshPropertyBuffer();

    virtual bool IsLoaded() const override;

protected:
    virtual void Load() override;
    virtual void Unload() override;

    PROPERTY()
    WeakObjectPtr<ShaderGraph> m_BaseGraph;

    PROPERTY()
    Array<String> m_InputNames;

    PROPERTY()
    Array<Vec4> m_InputValues;

    PROPERTY()
    Array<String> m_TextureNames;

    PROPERTY()
    Array<WeakObjectPtr<Texture2D>> m_TextureValues;

    SharedObjectPtr<UniformBuffer> m_PropertyBuffer;
    uint32_t m_BufferSize = 0;
    bool m_Loaded = false;
};
