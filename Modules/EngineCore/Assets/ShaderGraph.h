#pragma once
#include "Asset.h"
#include "Graph/NodeGraph.h"
#include "Rendering/ShaderGraphTypes.h"
#include "Rendering/ShaderTemplate.h"
#include "ShaderGraph.gen.h"

class Shader;
class Texture2D;
class UniformBuffer;
class ShaderGraphOutputNode;

struct ShaderGraphTextureBinding {
    uint32_t Binding = 0;
    String Name;
    Texture2D* Texture = nullptr;
};

class ShaderGraph : public Asset {
public:
    ARTIFACT_CLASS();

    virtual ~ShaderGraph() = default;

    static constexpr const char* DefaultTemplate = "/Shaders/Templates/Surface.glsl";

    static ShaderGraph* CreateEmpty(const String& InDirectory, const String& InName);

    NodeGraph* GetGraph() const { return m_Graph.Get(); }
    const String& GetTemplatePath() const { return m_TemplatePath; }
    const ShaderTemplate& GetTemplate() const { return m_Template; }
    void SetTemplatePath(const String& InPath);

    Shader* GetShader() const { return m_Shader.Get(); }
    UniformBuffer* GetPropertyBuffer() const { return m_PropertyBuffer.Get(); }
    Array<ShaderGraphTextureBinding> GetTextureBindings() const;

    /** Every value a material instance can override */
    const Array<ShaderGraphProperty>& GetInputs() const { return m_Inputs; }
    Vec4 GetInputValue(const String& InName) const;
    void SetInputValue(const String& InName, const Vec4& InValue);

    Array<String> GetStateNames() const { return m_Template.GetStateNames(); }
    Array<String> GetStateOptions(const String& InState) const { return m_Template.GetStateOptions(InState); }
    String GetStateValue(const String& InState) const;
    void SetStateValue(const String& InState, const String& InValue);

    bool Recompile(String& OutError);
    String GenerateSource(String& OutError) const;
    bool RegisterGeneratedSource();

    String GetShaderKey() const { return GetId().ToString(); }

    virtual String GetDisplayName() const override;
    virtual bool IsLoaded() const override;

protected:
    virtual void Load() override;
    virtual void Unload() override;

private:
    bool EnsureTemplateLoaded();
    void SyncGraph();
    void BuildInputs();
    void UploadPropertyBuffer();
    String BuildDeclarations() const;
    ShaderGraphOutputNode* FindOutputNode() const;

    PROPERTY()
    String m_TemplatePath = DefaultTemplate;

    PROPERTY()
    SharedObjectPtr<NodeGraph> m_Graph;

    PROPERTY()
    Array<String> m_InputNames;

    PROPERTY()
    Array<Vec4> m_InputValues;

    PROPERTY()
    Array<String> m_StateNames;

    PROPERTY()
    Array<String> m_StateValues;

    ShaderTemplate m_Template;
    String m_TemplateError;
    bool m_TemplateLoaded = false;

    Array<ShaderGraphProperty> m_Inputs;
    Array<String> m_InputErrors;
    Map<String, uint32_t> m_InputOffsets;
    uint32_t m_BlockSize = 0;
    uint32_t m_BufferSize = 0;

    SharedObjectPtr<Shader> m_Shader;
    SharedObjectPtr<UniformBuffer> m_PropertyBuffer;
};
