#pragma once
#include "CoreMinimal.h"
#include "ShaderGraphTypes.h"
#include "ShaderSource.h"

struct ShaderPropertyCode {
    String Prelude;
    String Expression;
};

struct ShaderPropertySite {
    String Identifier;
    String PropertyName;
    ShaderValueType Type = ShaderValueType::Float;
    ShaderStage Stage = ShaderStage::Vertex;
    uint32_t LineIndex = 0;
};

struct ShaderTemplateInfo {
    String Path;
    String DisplayName;
};

class ShaderTemplate {
public:
    static constexpr const char* Directive = "#shadergraph";
    static constexpr uint32_t MaterialTextureBase = 17;
    static constexpr uint32_t MaterialUniformBinding = 1;
    static constexpr uint32_t ShadowMapBinding = 2;

    static bool IsTemplateSource(const String& InSource);
    static Array<ShaderTemplateInfo> FindAll();

    static bool Parse(const String& InPath, ShaderTemplate& OutTemplate, String& OutError);
    static bool ParseSource(const String& InPath, const String& InSource, ShaderTemplate& OutTemplate, String& OutError);

    const String& GetPath() const { return m_Path; }
    const String& GetDisplayName() const { return m_DisplayName; }
    const Array<ShaderGraphProperty>& GetProperties() const { return m_Properties; }
    const ShaderGraphProperty* FindProperty(const String& InName) const;

    /** Values a render-state directive offers the graph. Empty means the template fixed it. */
    Array<String> GetStateOptions(const String& InState) const;
    Array<String> GetStateNames() const;
    String GetDefaultStateValue(const String& InState) const;

    /** InDeclarations is emitted at every #gen_buffers site, after the stage's own declarations. */
    String Expand(const Map<String, ShaderPropertyCode>& InCode, const Map<String, String>& InStates,
                  const String& InDeclarations) const;

    static String GetStageDeclarations(ShaderStage InStage);
    static String GetStageAliases(ShaderStage InStage);

private:
    String m_Path;
    String m_DisplayName;
    Array<String> m_Lines;
    Array<ShaderGraphProperty> m_Properties;
    Array<ShaderPropertySite> m_Sites;
    Map<String, Array<String>> m_StateOptions;
    Array<String> m_StateOrder;
};
