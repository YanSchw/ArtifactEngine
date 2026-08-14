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
struct DetailsEditHandler;
struct Property;

/** One line of the track list: a whole property, or one component of it while expanded. */
struct TimelineRow {
    int32_t Track = 0;
    int32_t Component = -1;
    int32_t Depth = 0;
    bool Expandable = false;
    bool Expanded = false;
    String Label;
};

/** What a row edits: the animated property, or one member of it. */
struct TimelineValueRef {
    Node* Target = nullptr;
    Property* Root = nullptr;
    Property* Leaf = nullptr;
    uint64_t Offset = 0;
};

/** The key editor of an AnimationEditorTab. */
class AnimationTimelineTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    static constexpr float LabelColumnWidth = 300.0f;
    static constexpr float ValueColumnWidth = 92.0f;
    static constexpr float IndentStep = 11.0f;
    static constexpr float RowHeight = 22.0f;
    static constexpr float RulerHeight = 24.0f;
    static constexpr float SummaryHeight = 18.0f;
    static constexpr float ScrollBarHeight = 10.0f;
    static constexpr float StripMargin = 10.0f;
    static constexpr float MinVisibleFrames = 2.0f;
    static constexpr float MaxZoomOutFactor = 8.0f;
    static constexpr float OverscrollFraction = 0.25f;

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
    void GetScrollDomain(float& OutStart, float& OutEnd) const;

    int32_t GetTrackCount() const;
    const AnimationTrack* GetTrack(int32_t InIndex) const;
    String GetTrackLabel(const AnimationTrack& InTrack) const;
    Node* GetTrackNode(const AnimationTrack& InTrack) const;

    int32_t GetRowCount() const { return m_Visible.Size(); }
    const TimelineRow* GetRow(int32_t InIndex) const;
    const AnimationTrack* GetRowTrack(int32_t InIndex) const;
    void ToggleExpanded(int32_t InIndex);

    TimelineValueRef ResolveRow(int32_t InIndex) const;
    VectorImage* GetRowIcon(int32_t InIndex) const;
    void BuildRowEditor(UINode& InHost, int32_t InIndex);

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
    float GetTotalFrames() const;
    /** Where the property sits in the node's Details layout, so tracks list in that order. */
    int32_t GetPropertyOrder(const AnimationTrack& InTrack) const;
    /** Writing through a timeline row keys the property at the playhead. */
    DetailsEditHandler MakeRowEditHandler(const TimelineValueRef& InRef);
    String RowValueText(int32_t InIndex) const;

    Array<TimelineRow> m_Visible;
    Array<String> m_ExpandedTracks;
    UIVStack* m_Rows = nullptr;
    UITextArea* m_FrameField = nullptr;
    int32_t m_SelectedKey = -1;
    float m_ViewStart = 0.0f;
    float m_ViewEnd = 0.0f;
};
