#pragma once
#include "MajorTab.h"
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "AnimationGraphEditorTab.gen.h"

class AnimationGraph;
class AnimationGraphView;
class AnimatorNode;
class UIVStack;
class VectorImage;

class AnimationGraphEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    AnimationGraphEditorTab();

    void OpenGraph(AnimationGraph* InGraph);
    AnimationGraph* GetGraph() const { return m_Graph.Get(); }
    AnimatorNode* GetLiveAnimator() const { return m_LiveAnimator.Get(); }
    void Save();

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    AnimatorNode* FindLiveAnimator() const;

    WeakObjectPtr<AnimationGraph> m_Graph;
    WeakObjectPtr<AnimatorNode> m_LiveAnimator;
    float m_LiveSearchCooldown = 0.0f;
};

class AnimationGraphCanvasTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    AnimationGraphCanvasTab();

    virtual String GetTabTitle() const override { return "Animation Graph"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnBind() override;

private:
    AnimationGraphEditorTab* GetEditor() const;

    AnimationGraphView* m_View = nullptr;
    WeakObjectPtr<AnimationGraph> m_ShownGraph;
};

class AnimationParametersTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    AnimationParametersTab();

    virtual String GetTabTitle() const override { return "Parameters"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnBind() override;

private:
    AnimationGraph* GetGraph() const;
    void Rebuild();
    void BuildRow(int32_t InIndex);
    void Edit(const String& InTitle, const std::function<void(AnimationGraph&)>& InEdit);

    UIVStack* m_Rows = nullptr;
    WeakObjectPtr<AnimationGraph> m_BuiltFor;
    int32_t m_BuiltCount = -1;
};
