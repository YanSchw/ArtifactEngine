#pragma once
#include "Material.h"
#include "Graph/NodeGraph.h"
#include "Rendering/ShaderGraphTypes.h"
#include "Rendering/ShaderTemplate.h"
#include "ShaderGraph.gen.h"

class Shader;
class ShaderGraphOutputNode;

class ShaderGraph : public Material {
public:
    ARTIFACT_CLASS();

    virtual ~ShaderGraph() = default;

    static constexpr const char* DefaultTemplate = "/Shaders/Templates/Surface.glsl";

    static ShaderGraph* CreateEmpty(const String& InDirectory, const String& InName);

    NodeGraph* GetGraph() const { return m_Graph.Get(); }
    const String& GetTemplatePath() const { return m_TemplatePath; }
    const ShaderTemplate& GetTemplate() const { return m_Template; }
    void SetTemplatePath(const String& InPath);

    virtual ShaderGraph* GetBaseGraph() const override { return const_cast<ShaderGraph*>(this); }

    Shader* GetCompiledShader() const { return m_Shader.Get(); }

    /** The inputs this graph publishes: its template's properties plus every exposed value node. */
    const Array<ShaderGraphProperty>& GetDeclaredInputs() const { return m_Inputs; }
    uint32_t GetPropertyBlockSize() const { return m_BlockSize; }
    bool FindInputOffset(const String& InName, uint32_t& OutOffset) const;
    Array<MaterialTextureBinding> GetGraphTextureBindings() const;

    Array<String> GetStateNames() const { return m_Template.GetStateNames(); }
    Array<String> GetStateOptions(const String& InState) const { return m_Template.GetStateOptions(InState); }
    String GetStateValue(const String& InState) const;
    void SetStateValue(const String& InState, const String& InValue);

    bool Recompile(String& OutError);
    String GenerateSource(String& OutError) const;
    bool RegisterGeneratedSource();

    String GetShaderKey() const { return GetId().ToString(); }

    virtual bool IsLoaded() const override;

protected:
    virtual void Load() override;
    virtual void Unload() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

private:
    bool EnsureTemplateLoaded();
    void SyncGraph();
    void BuildInputs();
    void BuildInputLayout();
    String BuildDeclarations() const;
    ShaderGraphOutputNode* FindOutputNode() const;

    PROPERTY()
    String m_TemplatePath = DefaultTemplate;

    PROPERTY()
    SharedObjectPtr<NodeGraph> m_Graph;

    PROPERTY()
    Array<ShaderGraphProperty> m_Inputs;

    PROPERTY()
    Array<String> m_StateNames;

    PROPERTY()
    Array<String> m_StateValues;

    ShaderTemplate m_Template;
    String m_TemplateError;
    bool m_TemplateLoaded = false;

    Array<String> m_InputErrors;
    Map<String, uint32_t> m_InputOffsets;
    uint32_t m_BlockSize = 0;

    SharedObjectPtr<Shader> m_Shader;
};
