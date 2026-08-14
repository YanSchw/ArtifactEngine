#include "AnimationTimelineTab.h"
#include "AnimationTimelineWidgets.h"
#include "AnimationEditorTab.h"
#include "Details/DetailsCustomization.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIContextMenu.h"
#include "UI/UIMenuModel.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/Node.h"
#include "Assets/Animation.h"
#include "Object/Property.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include <cstring>
#include "InputSystem/KeyboardDevice.h"
#include <cmath>
#include <cstdlib>

static const Vec4 s_RecordingBar = HexColor(0x4A1A18);
static const int32_t s_NiceSteps[] = { 1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 1200 };

AnimationTimelineTab::AnimationTimelineTab() {
    UIVStack* layout = Add<UIVStack>();
    layout->Fill();

    BuildTransport(*layout);

    AnimationTimelineRuler* ruler = layout->Add<AnimationTimelineRuler>();
    ruler->Owner = this;
    ruler->Size = { 1.0_rel, UIValue(RulerHeight) };

    AnimationTimelineSummary* summary = layout->Add<AnimationTimelineSummary>();
    summary->Owner = this;
    summary->Size = { 1.0_rel, UIValue(SummaryHeight) };

    UIScrollArea* scroll = layout->Add<UIScrollArea>();
    scroll->Size = { 1.0_rel, 1.0_rel };

    UIVStack* rows = scroll->Add<UIVStack>();
    rows->Anchor = rows->Pivot = Vec2(0.0f);
    rows->Position = Vec2(0.0f);
    rows->Size = { 1.0_rel, 0.0_px };
    rows->Bind = [this] { RefreshRows(); };
    m_Rows = rows;

    AnimationTimelineScrollBar* scrollBar = layout->Add<AnimationTimelineScrollBar>();
    scrollBar->Owner = this;
    scrollBar->Size = { 1.0_rel, UIValue(ScrollBarHeight) };

    BuildFooter(*layout);
}

void AnimationTimelineTab::OnUIUpdate(const UIFrameContext& InContext) {
    (void)InContext;
    if (m_ViewEnd <= m_ViewStart) {
        FrameAll();
    } else {
        ClampView();
    }

    Animation* animation = GetAnimation();
    if (animation && m_SelectedKey >= animation->GetKeys().Size()) {
        m_SelectedKey = -1;
    }
    HandleShortcuts();
}

void AnimationTimelineTab::HandleShortcuts() {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    AnimationEditorTab* editor = GetEditor();
    if (!keyboard || !editor || !AcceptsShortcuts()) {
        return;
    }

    if (keyboard->IsDown(KeyCode::Space)) {
        editor->SetPlaying(!editor->IsPlaying());
    }
    if (keyboard->IsDown(KeyCode::Delete) || keyboard->IsDown(KeyCode::Backspace)) {
        DeleteSelectedKey();
    }
    if (keyboard->IsDown(KeyCode::F)) {
        FrameAll();
    }

    const bool jumpToKey = keyboard->IsPressed(KeyCode::LeftAlt) || keyboard->IsPressed(KeyCode::RightAlt);
    if (keyboard->IsDown(KeyCode::Left)) {
        editor->SetPreviewing(true);
        jumpToKey ? StepToAdjacentKey(-1) : editor->SetFrame(editor->GetFrame() - 1);
    }
    if (keyboard->IsDown(KeyCode::Right)) {
        editor->SetPreviewing(true);
        jumpToKey ? StepToAdjacentKey(1) : editor->SetFrame(editor->GetFrame() + 1);
    }
}

VectorImage* AnimationTimelineTab::GetTabIcon() const {
    return EditorIcons::Animation();
}

AnimationEditorTab* AnimationTimelineTab::GetEditor() const {
    return GetMajorTab() ? GetMajorTab()->As<AnimationEditorTab>() : nullptr;
}

Animation* AnimationTimelineTab::GetAnimation() const {
    AnimationEditorTab* editor = GetEditor();
    return editor ? editor->GetAnimation() : nullptr;
}

UIRectF AnimationTimelineTab::StripOf(const UIRectF& InRow) {
    const float width = InRow.Size.x - LabelColumnWidth;
    return UIRectF(InRow.Position + Vec2(LabelColumnWidth, 0.0f),
                   Vec2(width > 1.0f ? width : 1.0f, InRow.Size.y));
}

static float UsableWidth(const UIRectF& InStrip) {
    const float usable = InStrip.Size.x - 2.0f * AnimationTimelineTab::StripMargin;
    return usable > 1.0f ? usable : 1.0f;
}

float AnimationTimelineTab::FrameToX(const UIRectF& InStrip, float InFrame) const {
    const float span = m_ViewEnd - m_ViewStart;
    const float alpha = span > 0.0f ? (InFrame - m_ViewStart) / span : 0.0f;
    return InStrip.Min().x + StripMargin + alpha * UsableWidth(InStrip);
}

float AnimationTimelineTab::XToFrame(const UIRectF& InStrip, float InX) const {
    const float alpha = (InX - InStrip.Min().x - StripMargin) / UsableWidth(InStrip);
    return m_ViewStart + alpha * (m_ViewEnd - m_ViewStart);
}

void AnimationTimelineTab::ClampView() {
    Animation* animation = GetAnimation();
    const float total = animation ? (float)animation->GetFrameCount() : MinVisibleFrames;
    const float maxSpan = total * MaxZoomOutFactor;

    float span = m_ViewEnd - m_ViewStart;
    span = span < MinVisibleFrames ? MinVisibleFrames : (span > maxSpan ? maxSpan : span);

    // The view may sit past either end, but never so far that the animation leaves the screen.
    const float minStart = -total * 0.5f;
    const float maxStart = total;
    m_ViewStart = m_ViewStart < minStart ? minStart : (m_ViewStart > maxStart ? maxStart : m_ViewStart);
    m_ViewEnd = m_ViewStart + span;
}

float AnimationTimelineTab::GetOutOfRangeEndX(const UIRectF& InStrip) const {
    Animation* animation = GetAnimation();
    return FrameToX(InStrip, animation ? (float)animation->GetFrameCount() : 0.0f);
}

void AnimationTimelineTab::FrameAll() {
    Animation* animation = GetAnimation();
    m_ViewStart = 0.0f;
    m_ViewEnd = animation ? (float)animation->GetFrameCount() : MinVisibleFrames;
    ClampView();
}

void AnimationTimelineTab::SetViewStart(float InStart) {
    const float span = m_ViewEnd - m_ViewStart;
    m_ViewStart = InStart;
    m_ViewEnd = InStart + span;
    ClampView();
}

void AnimationTimelineTab::ZoomAt(const UIRectF& InStrip, float InX, float InSteps) {
    const float span = m_ViewEnd - m_ViewStart;
    if (span <= 0.0f) {
        FrameAll();
        return;
    }
    const float pivot = XToFrame(InStrip, InX);
    const float alpha = (pivot - m_ViewStart) / span;

    m_ViewStart = pivot - alpha * span * std::pow(1.2f, -InSteps);
    m_ViewEnd = m_ViewStart + span * std::pow(1.2f, -InSteps);
    ClampView();
}

void AnimationTimelineTab::PanByPixels(const UIRectF& InStrip, float InPixels) {
    SetViewStart(m_ViewStart - InPixels / UsableWidth(InStrip) * (m_ViewEnd - m_ViewStart));
}

int32_t AnimationTimelineTab::LabelledFrameStep(const UIRectF& InStrip) const {
    const float wanted = (m_ViewEnd - m_ViewStart) * 56.0f / UsableWidth(InStrip);
    for (int32_t step : s_NiceSteps) {
        if ((float)step >= wanted) {
            return step;
        }
    }
    return s_NiceSteps[sizeof(s_NiceSteps) / sizeof(s_NiceSteps[0]) - 1];
}

int32_t AnimationTimelineTab::GetTrackCount() const {
    Animation* animation = GetAnimation();
    return animation ? animation->GetTracks().Size() : 0;
}

const AnimationTrack* AnimationTimelineTab::GetTrack(int32_t InIndex) const {
    Animation* animation = GetAnimation();
    if (!animation || InIndex < 0 || InIndex >= animation->GetTracks().Size()) {
        return nullptr;
    }
    return &animation->GetTracks()[InIndex];
}

Node* AnimationTimelineTab::GetTrackNode(const AnimationTrack& InTrack) const {
    AnimationEditorTab* editor = GetEditor();
    Node* root = editor ? editor->GetAnimationRoot() : nullptr;
    return root ? Animation::ResolvePath(*root, InTrack.Path) : nullptr;
}

String AnimationTimelineTab::GetTrackLabel(const AnimationTrack& InTrack) const {
    Node* node = GetTrackNode(InTrack);
    const String name = node ? node->GetName() : String("(missing)");
    return name + " : " + DetailsCustomization::PrettyPropertyName(InTrack.PropertyName);
}

static const char* s_ExpandableStructs[] = { "Vec2", "Vec3", "Vec4", "Color" };

static Array<Property*> ComponentsOf(Property* InProperty) {
    StructProperty* structProperty = Cast<StructProperty>(InProperty);
    for (const char* expandable : s_ExpandableStructs) {
        if (structProperty && structProperty->InnerStructTypename == expandable) {
            return Property::GetTypeProperties(structProperty->InnerStructTypename);
        }
    }
    return Array<Property*>();
}

static String TrackKey(const AnimationTrack& InTrack) {
    String key;
    for (int32_t index : InTrack.Path) {
        key += std::to_string(index) + "/";
    }
    return key + InTrack.PropertyName;
}

const TimelineRow* AnimationTimelineTab::GetRow(int32_t InIndex) const {
    return (InIndex >= 0 && InIndex < m_Visible.Size()) ? &m_Visible[InIndex] : nullptr;
}

const AnimationTrack* AnimationTimelineTab::GetRowTrack(int32_t InIndex) const {
    const TimelineRow* row = GetRow(InIndex);
    return row ? GetTrack(row->Track) : nullptr;
}

void AnimationTimelineTab::ToggleExpanded(int32_t InIndex) {
    const AnimationTrack* track = GetRowTrack(InIndex);
    if (!track) {
        return;
    }
    const String key = TrackKey(*track);
    if (m_ExpandedTracks.Contains(key)) {
        m_ExpandedTracks.Remove(key);
    } else {
        m_ExpandedTracks.Add(key);
    }
}

void AnimationTimelineTab::RebuildVisible() {
    m_Visible.Clear();
    Animation* animation = GetAnimation();
    if (!animation) {
        return;
    }

    for (int32_t index = 0; index < animation->GetTracks().Size(); index++) {
        const AnimationTrack& track = animation->GetTracks()[index];
        Node* node = GetTrackNode(track);
        Property* property = node ? Property::FindTypeProperty(node->GetClass(), track.PropertyName) : nullptr;
        const Array<Property*> components = ComponentsOf(property);

        TimelineRow row;
        row.Track = index;
        row.Label = GetTrackLabel(track);
        row.Expandable = !components.IsEmpty();
        row.Expanded = row.Expandable && m_ExpandedTracks.Contains(TrackKey(track));
        m_Visible.Add(row);

        if (!row.Expanded) {
            continue;
        }
        for (int32_t component = 0; component < components.Size(); component++) {
            TimelineRow leaf;
            leaf.Track = index;
            leaf.Component = component;
            leaf.Label = DetailsCustomization::PrettyPropertyName(track.PropertyName) + "." + components[component]->Name;
            m_Visible.Add(leaf);
        }
    }
}

TimelineValueRef AnimationTimelineTab::ResolveRow(int32_t InIndex) const {
    TimelineValueRef ref;
    const TimelineRow* row = GetRow(InIndex);
    const AnimationTrack* track = GetRowTrack(InIndex);
    Node* node = track ? GetTrackNode(*track) : nullptr;
    if (!row || !node) {
        return ref;
    }

    Property* root = Property::FindTypeProperty(node->GetClass(), track->PropertyName);
    if (!root) {
        return ref;
    }
    ref.Target = node;
    ref.Root = root;

    char* base = (char*)root->GetValuePtr(node);
    if (row->Component < 0) {
        if (Cast<FloatProperty>(root) || Cast<IntProperty>(root)) {
            ref.Numeric = root;
            ref.Address = base;
        }
        return ref;
    }

    const Array<Property*> components = ComponentsOf(root);
    if (row->Component < components.Size()) {
        ref.Numeric = components[row->Component];
        ref.Address = base + components[row->Component]->Offset;
    }
    return ref;
}

double AnimationTimelineTab::ReadRowValue(const TimelineValueRef& InRef) const {
    if (!InRef.Address) {
        return 0.0;
    }
    if (FloatProperty* number = Cast<FloatProperty>(InRef.Numeric)) {
        return number->IsDouble ? *(double*)InRef.Address : (double)*(float*)InRef.Address;
    }
    if (IntProperty* number = Cast<IntProperty>(InRef.Numeric)) {
        switch (number->NumBits) {
            case 8:  return number->IsUnsigned ? (double)*(uint8_t*)InRef.Address : (double)*(int8_t*)InRef.Address;
            case 16: return number->IsUnsigned ? (double)*(uint16_t*)InRef.Address : (double)*(int16_t*)InRef.Address;
            case 32: return number->IsUnsigned ? (double)*(uint32_t*)InRef.Address : (double)*(int32_t*)InRef.Address;
            default: return number->IsUnsigned ? (double)*(uint64_t*)InRef.Address : (double)*(int64_t*)InRef.Address;
        }
    }
    return 0.0;
}

void AnimationTimelineTab::WriteRowValue(const TimelineValueRef& InRef, double InValue) {
    if (!InRef.Address || !InRef.Target) {
        return;
    }
    if (FloatProperty* number = Cast<FloatProperty>(InRef.Numeric)) {
        if (number->IsDouble) {
            *(double*)InRef.Address = InValue;
        } else {
            *(float*)InRef.Address = (float)InValue;
        }
    } else if (IntProperty* number = Cast<IntProperty>(InRef.Numeric)) {
        const int64_t rounded = (int64_t)std::llround(InValue);
        std::memcpy(InRef.Address, &rounded, number->NumBits / 8);
    } else {
        return;
    }

    RecordEdit("Edit " + DetailsCustomization::PrettyPropertyName(InRef.Root->Name), InRef.Target);
    InRef.Root->NotifyChanged(InRef.Target);
    DetailsCustomization::NotifyPropertyEdited(GetMajorTab(), InRef.Target, InRef.Root->Name);
}

String AnimationTimelineTab::RowValueText(int32_t InIndex) const {
    const TimelineValueRef ref = ResolveRow(InIndex);
    const TimelineRow* row = GetRow(InIndex);
    if (!ref.Root || ref.Numeric || !row || row->Expandable) {
        return String();
    }
    if (Cast<StructProperty>(ref.Root) || Cast<ArrayProperty>(ref.Root)) {
        return String();
    }

    const String text = JsonSerializer::SerializeProperty(ref.Root, ref.Root->GetValuePtr(ref.Target)).dump();
    return text.size() > 10 ? text.substr(0, 9) + "..." : text;
}

void AnimationTimelineTab::ScrubTo(const UIRectF& InStrip, float InX) {
    if (AnimationEditorTab* editor = GetEditor()) {
        editor->SetPlaying(false);
        editor->SetPreviewing(true);
        editor->SetFrame((int32_t)std::lround(XToFrame(InStrip, InX)));
    }
}

void AnimationTimelineTab::DeleteSelectedKey() {
    Animation* animation = GetAnimation();
    if (animation && m_SelectedKey >= 0) {
        animation->RemoveKey(m_SelectedKey);
        m_SelectedKey = -1;
    }
}

void AnimationTimelineTab::StepToAdjacentKey(int32_t InDirection) {
    AnimationEditorTab* editor = GetEditor();
    Animation* animation = GetAnimation();
    if (!editor || !animation) {
        return;
    }

    const int32_t current = editor->GetFrame();
    int32_t best = current;
    for (const AnimationKey& key : animation->GetKeys()) {
        const bool ahead = InDirection > 0 ? key.Frame > current : key.Frame < current;
        const bool closer = best == current || (InDirection > 0 ? key.Frame < best : key.Frame > best);
        if (ahead && closer) {
            best = key.Frame;
        }
    }
    editor->SetPreviewing(true);
    editor->SetFrame(best);
}

void AnimationTimelineTab::OpenAddPropertyMenu(UINode& InAnchor) {
    AnimationEditorTab* editor = GetEditor();
    Animation* animation = GetAnimation();
    Node* root = editor ? editor->GetAnimationRoot() : nullptr;
    if (!editor || !animation || !root) {
        return;
    }

    Node* node = Cast<Node>(editor->GetSoleSelection());
    if (!node) {
        node = root;
    }

    const WeakObjectPtr<Node> target = node;
    UIMenuModel menu;
    menu.Searchable("Search properties");
    menu.Section(node->GetName());

    for (Property* property : Property::GetAllTypeProperties(node->GetClass())) {
        if (!Animation::IsAnimatable(property)) {
            continue;
        }
        const String propertyName = property->Name;
        Array<int32_t> path;
        const bool tracked = Animation::MakePath(*root, *node, path)
                          && animation->FindTrack(path, propertyName) >= 0;

        menu.Item(DetailsCustomization::PrettyPropertyName(propertyName), [this, target, propertyName] {
            AnimationEditorTab* owner = GetEditor();
            if (owner && target.Get()) {
                owner->SetPreviewing(true);
                owner->KeyProperty(*target.Get(), propertyName);
            }
        }).Checked(tracked);
    }

    UIContextMenu::OpenUnder(InAnchor, menu);
}

void AnimationTimelineTab::BuildTransport(UINode& InParent) {
    UIQuad* bar = InParent.Add<UIQuad>();
    bar->Size = { 1.0_rel, 30.0_px };
    bar->Color = EditorStyle::ToolBar;
    bar->Bind = [this, bar] {
        AnimationEditorTab* editor = GetEditor();
        bar->Color = (editor && editor->IsRecording()) ? s_RecordingBar : EditorStyle::ToolBar;
    };

    UIHStack* row = bar->Add<UIHStack>();
    row->Fill();
    row->Padding = UIPadding(6.0f, 4.0f);
    row->Gap = 4.0f;

    UIButton* preview = row->Add<UIButton>();
    preview->Size = { 68.0_px, 1.0_rel };
    preview->SetCaption("Preview");
    EditorStyle::ApplyButtonStyle(*preview, EditorStyle::FontSize - 1.0f);
    preview->Clicked = [this] {
        if (AnimationEditorTab* editor = GetEditor()) {
            editor->SetPreviewing(!editor->IsPreviewing());
        }
    };
    preview->Bind = [this, preview] {
        AnimationEditorTab* editor = GetEditor();
        const bool on = editor && editor->IsPreviewing();
        preview->NormalColor = on ? EditorStyle::Accent : EditorStyle::Button;
        preview->HoverColor = on ? EditorStyle::AccentBright : EditorStyle::ButtonHover;
    };

    const auto iconButton = [&row](VectorImage* InIcon, float InRotation, const Vec4& InTint, std::function<void()> InAction) {
        UIButton* button = row->Add<UIButton>();
        button->Size = { 26.0_px, 1.0_rel };
        button->NormalColor = EditorStyle::Button;
        button->HoverColor = EditorStyle::ButtonHover;
        button->PressedColor = EditorStyle::ButtonPressed;
        button->Clicked = std::move(InAction);
        UISvg* icon = button->Add<UISvg>();
        icon->Center(Vec2(11.0f, 11.0f));
        icon->Image = InIcon;
        icon->Tint = InTint;
        icon->Rotation = Vec3(0.0f, 0.0f, InRotation);
        return button;
    };

    iconButton(EditorIcons::ArrowLeft(), 0.0f, EditorStyle::Text, [this] {
        if (AnimationEditorTab* editor = GetEditor()) {
            editor->SetPlaying(false);
            editor->SetPreviewing(true);
            editor->SetFrame(0);
        }
    });
    iconButton(EditorIcons::ArrowRight(), 180.0f, EditorStyle::Text, [this] { StepToAdjacentKey(-1); });

    UIButton* play = iconButton(EditorIcons::Play(), 0.0f, EditorStyle::TransformY, [this] {
        if (AnimationEditorTab* editor = GetEditor()) {
            editor->SetPlaying(!editor->IsPlaying());
        }
    });
    play->Bind = [this, play] {
        AnimationEditorTab* editor = GetEditor();
        play->NormalColor = (editor && editor->IsPlaying()) ? EditorStyle::Accent : EditorStyle::Button;
    };

    iconButton(EditorIcons::ArrowRight(), 0.0f, EditorStyle::Text, [this] { StepToAdjacentKey(1); });
    iconButton(EditorIcons::ArrowLeft(), 180.0f, EditorStyle::Text, [this] {
        AnimationEditorTab* editor = GetEditor();
        Animation* animation = GetAnimation();
        if (editor && animation) {
            editor->SetPlaying(false);
            editor->SetPreviewing(true);
            editor->SetFrame(animation->GetFrameCount());
        }
    });

    const auto numberField = [&row](float InWidth, std::function<String()> InRead, std::function<void(int32_t)> InWrite) {
        UITextArea* field = row->Add<UITextArea>();
        field->Size = { UIValue(InWidth), 1.0_rel };
        field->SingleLine = true;
        field->FontSize = EditorStyle::FontSize - 1.0f;
        field->TextColor = EditorStyle::TextBright;
        field->CaretColor = EditorStyle::TextBright;
        field->BackgroundColor = EditorStyle::PanelDark;
        field->FocusedBorderColor = EditorStyle::Accent;
        field->Padding = UIPadding(5.0f, 0.0f);
        field->Submitted = [InWrite](const String& InText) { InWrite(std::atoi(InText.c_str())); };
        field->Bind = [field, InRead] {
            if (!field->IsFocused()) {
                field->Text = InRead();
            }
        };
        return field;
    };
    const auto caption = [&row](const String& InText) {
        UILabel* label = row->Add<UILabel>();
        label->Size = { UIValue((float)InText.size() * 8.0f + 8.0f), 1.0_rel };
        label->FontSize = EditorStyle::FontSize - 1.0f;
        label->Color = EditorStyle::TextDim;
        label->HAlign = UIHAlign::Right;
        label->VAlign = UIVAlign::Middle;
        label->Text = InText;
    };

    m_FrameField = numberField(56.0f,
        [this] { AnimationEditorTab* editor = GetEditor(); return editor ? std::to_string(editor->GetFrame()) : String(); },
        [this](int32_t InValue) {
            if (AnimationEditorTab* editor = GetEditor()) {
                editor->SetPlaying(false);
                editor->SetPreviewing(true);
                editor->SetFrame(InValue);
            }
        });

    caption("Length");
    numberField(56.0f,
        [this] { Animation* animation = GetAnimation(); return animation ? std::to_string(animation->GetFrameCount()) : String(); },
        [this](int32_t InValue) { if (Animation* animation = GetAnimation()) { animation->SetFrameCount(InValue); } });

    caption("FPS");
    numberField(48.0f,
        [this] { Animation* animation = GetAnimation(); return animation ? std::to_string((int32_t)animation->GetFrameRate()) : String(); },
        [this](int32_t InValue) { if (Animation* animation = GetAnimation()) { animation->SetFrameRate((float)InValue); } });

    UILabel* summary = row->Add<UILabel>();
    summary->Size = { 1.0_rel, 1.0_rel };
    summary->Padding = UIPadding(10.0f, 0.0f);
    summary->FontSize = EditorStyle::FontSize - 1.0f;
    summary->Color = EditorStyle::TextDim;
    summary->VAlign = UIVAlign::Middle;
    summary->Bind = [this, summary] {
        Animation* animation = GetAnimation();
        summary->Text = animation ? std::to_string(animation->GetTracks().Size()) + " properties, "
                                    + std::to_string(animation->GetKeys().Size()) + " keys"
                                  : String();
    };
}

void AnimationTimelineTab::BuildFooter(UINode& InParent) {
    UIQuad* footer = InParent.Add<UIQuad>();
    footer->Size = { 1.0_rel, 28.0_px };
    footer->Color = EditorStyle::BottomBar;

    UIButton* add = footer->Add<UIButton>();
    add->Anchor = add->Pivot = Vec2(0.0f, 0.5f);
    add->Position = Vec2(8.0f, 0.0f);
    add->Size = Vec2(LabelColumnWidth - 16.0f, 20.0f);
    add->SetCaption("Add Property");
    EditorStyle::ApplyButtonStyle(*add, EditorStyle::FontSize - 1.0f);
    add->Clicked = [this, add] { OpenAddPropertyMenu(*add); };
}

void AnimationTimelineTab::RefreshRows() {
    RebuildVisible();
    const int32_t want = m_Visible.Size();

    int32_t have = (int32_t)m_Rows->GetChildCount();
    while (have < want) {
        AnimationTimelineRow* row = m_Rows->Add<AnimationTimelineRow>();
        row->Owner = this;
        row->RowIndex = have;
        row->Size = { 1.0_rel, UIValue(RowHeight) };
        have++;
    }
    while (have > want) {
        delete m_Rows->GetChild(have - 1);
        have--;
    }
    m_Rows->Size = { 1.0_rel, UIValue((float)want * RowHeight) };
}
