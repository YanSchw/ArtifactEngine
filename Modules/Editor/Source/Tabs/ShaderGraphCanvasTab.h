#pragma once
#include "MinorTab.h"
#include "ShaderGraphCanvasTab.gen.h"

class GraphEditorView;
class ShaderGraph;
class VectorImage;

class ShaderGraphCanvasTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    ShaderGraphCanvasTab();

    virtual String GetTabTitle() const override { return "Shader Graph"; }
    virtual VectorImage* GetTabIcon() const override;

    void SetShaderGraph(ShaderGraph* InShaderGraph);
    GraphEditorView* GetView() const { return m_View; }

private:
    GraphEditorView* m_View = nullptr;
};
