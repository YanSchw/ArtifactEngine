#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "ShaderGraphEditorTab.gen.h"

class ShaderGraph;
class ShaderGraphCanvasTab;
class ShaderGraphPreviewTab;
class VectorImage;
class UILabel;

class ShaderGraphEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    ShaderGraphEditorTab();

    void OpenShaderGraph(ShaderGraph* InShaderGraph);
    ShaderGraph* GetShaderGraph() const { return m_ShaderGraph.Get(); }

    void RequestRecompile();
    void Save();

    const String& GetCompileError() const { return m_CompileError; }

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual void OnObjectEdited(Object* InObject) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void Recompile();
    void BuildTemplateDropdown(UINode& InToolBar);
    void BuildStateDropdowns(UINode& InToolBar);
    void RefreshStatusLabel();

    WeakObjectPtr<ShaderGraph> m_ShaderGraph;
    ShaderGraphCanvasTab* m_Canvas = nullptr;
    ShaderGraphPreviewTab* m_Preview = nullptr;
    WeakObjectPtr<UILabel> m_StatusLabel;

    String m_CompileError;
    bool m_RecompilePending = false;
};
