#pragma once
#include "MinorTab.h"
#include "AnimationTimelineTab.gen.h"

class Animation;
class AnimationEditorTab;
class Node;
class UIVStack;
class UITextArea;
class VectorImage;
struct AnimationTrack;
struct Property;

/** One line of the track list: a whole property, or one component of it while expanded. */
struct TimelineRow {
    int32_t Track = 0;
    int32_t Component = -1;
    bool Expandable = false;
    bool Expanded = false;
    String Label;
};

/** Where a row's number lives, so it can be shown and edited in place. */
struct TimelineValueRef {
    Node* Target = nullptr;
    Property* Root = nullptr;
    Property* Numeric = nullptr;
    char* Address = nullptr;
};

/** The key editor of an AnimationEditorTab. */
class AnimationTimelineTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    static constexpr float LabelColumnWidth = 240.0f;
    static constexpr float ValueColumnWidth = 62.0f;
    static constexpr float IndentStep = 14.0f;
    static constexpr float RowHeight = 20.0f;
    static constexpr float RulerHeight = 24.0f;
    static constexpr float SummaryHeight = 18.0f;
    static constexpr float ScrollBarHeight = 10.0f;
    static constexpr float StripMargin = 10.0f;
    static constexpr float MinVisibleFrames = 2.0f;
    static constexpr float MaxZoomOutFactor = 8.0f;

    AnimationTimelineTab();

    virtual String GetTabTitle() const override { return "Timeline"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

    AnimationEditorTab* GetEditor() const;
    Animation* GetAnimation() const;

    float FrameToX(const UIRectF& InStrip, float InFrame) const;
    float XToFrame(const UIRectF& InStrip, float InX) const;
    int32_t LabelledFrameStep(const UIRectF& InStrip) const;
    static UIRectF StripOf(const UIRectF& InRow);

    float GetViewStart() const { return m_ViewStart; }
    float GetViewEnd() const { return m_ViewEnd; }
    void SetViewStart(float InStart);
    void ZoomAt(const UIRectF& InStrip, float InX, float InSteps);
    void PanByPixels(const UIRectF& InStrip, float InPixels);
    void FrameAll();

    int32_t GetTrackCount() const;
    const AnimationTrack* GetTrack(int32_t InIndex) const;
    String GetTrackLabel(const AnimationTrack& InTrack) const;
    Node* GetTrackNode(const AnimationTrack& InTrack) const;

    int32_t GetRowCount() const { return m_Visible.Size(); }
    const TimelineRow* GetRow(int32_t InIndex) const;
    const AnimationTrack* GetRowTrack(int32_t InIndex) const;
    void ToggleExpanded(int32_t InIndex);

    TimelineValueRef ResolveRow(int32_t InIndex) const;
    double ReadRowValue(const TimelineValueRef& InRef) const;
    void WriteRowValue(const TimelineValueRef& InRef, double InValue);
    String RowValueText(int32_t InIndex) const;

    /** Frames outside [0, length] are shown, dimmed, once the view is zoomed out past the end. */
    float GetOutOfRangeStartX(const UIRectF& InStrip) const { return FrameToX(InStrip, 0.0f); }
    float GetOutOfRangeEndX(const UIRectF& InStrip) const;

    int32_t GetSelectedKey() const { return m_SelectedKey; }
    void SelectKey(int32_t InKey) { m_SelectedKey = InKey; }
    void DeleteSelectedKey();

    void ScrubTo(const UIRectF& InStrip, float InX);
    void StepToAdjacentKey(int32_t InDirection);
    void OpenAddPropertyMenu(UINode& InAnchor);

private:
    void BuildTransport(UINode& InParent);
    void BuildFooter(UINode& InParent);
    void RefreshRows();
    void RebuildVisible();
    void ClampView();
    void HandleShortcuts();

    Array<TimelineRow> m_Visible;
    Array<String> m_ExpandedTracks;
    UIVStack* m_Rows = nullptr;
    UITextArea* m_FrameField = nullptr;
    int32_t m_SelectedKey = -1;
    float m_ViewStart = 0.0f;
    float m_ViewEnd = 0.0f;
};
