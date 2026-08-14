#include "AnimationEditorTab.h"
#include "AnimationTimelineTab.h"
#include "OutlinerTab.h"
#include "DetailsTab.h"
#include "ViewportTab.h"
#include "UI/UIDockArea.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIDropdown.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/Node.h"
#include "GameFramework/Component.h"
#include "Assets/Animation.h"
#include "Assets/AssetManager.h"
#include "Assets/NodeRecord.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include "Core/Log.h"
#include <cmath>
#include <memory>

static const Vec4 s_RecordColor = HexColor(0xC42B1C);

AnimationEditorTab::AnimationEditorTab() {
    SetEditedWorld(new World());

    UIDockArea* area = GetDockArea();
    m_Viewport = area->DockNew<ViewportTab>(UIDockSlot::Center);
    m_Timeline = area->DockNew<AnimationTimelineTab>(UIDockSlot::Bottom, nullptr, 0.5f);

    OutlinerTab* outliner = area->DockNew<OutlinerTab>(UIDockSlot::Right, m_Viewport->GetDockNode(), 0.5f);
    outliner->SetMode(OutlinerMode::Animation);
    area->DockNew<DetailsTab>(UIDockSlot::Right, outliner->GetDockNode(), 0.5f);
}

void AnimationEditorTab::DestroyInstance() {
    ClearSelection();
    m_PreviewPose.Clear();
    m_Previewing = false;
    m_Recording = false;
    m_Playing = false;
    if (Node* previous = m_Instance.Get()) {
        previous->Destroy();
        GetEditedWorld()->ResolvePendingKills();
    }
    m_Instance = nullptr;
    m_AnimationRoot = nullptr;
}

void AnimationEditorTab::BuildInstanceFrom(NodeRecord& InRecord) {
    DestroyInstance();

    Node* instance = GetEditedWorld()->Spawn(Class(InRecord.ClassName));
    if (!instance) {
        AE_ERROR("'{0}' cannot be instantiated as an Animation placeholder", InRecord.ClassName);
        return;
    }

    InRecord.Apply(*instance);
    m_Instance = instance;

    Animation* animation = m_Animation.Get();
    if (animation) {
        instance->SetName(animation->GetDisplayName());
        m_AnimationRoot = Animation::ResolvePath(*instance, animation->GetRootPath());
    }

    SetSelection(instance);
}

void AnimationEditorTab::OpenAnimation(Animation* InAnimation) {
    DestroyInstance();
    m_Animation = InAnimation;
    m_FramePosition = 0.0f;
    m_Previewing = false;
    m_Playing = false;
    m_Recording = false;

    if (!InAnimation) {
        return;
    }

    AssetManager::Get().LoadAsset(InAnimation);
    NodeRecord* record = InAnimation->GetRoot();
    if (!record) {
        AE_WARN("Animation '{0}' has no placeholder hierarchy", InAnimation->GetDisplayName());
        return;
    }

    BuildInstanceFrom(*record);
}

Class AnimationEditorTab::GetPlaceholderClass() const {
    if (Node* instance = m_Instance.Get()) {
        return instance->GetSerializedClass();
    }
    Animation* animation = m_Animation.Get();
    return animation ? animation->GetRootClass() : Class::None;
}

Array<Class> AnimationEditorTab::GetSelectablePlaceholderClasses() const {
    Array<Class> classes = Class::GetSubclassesOf(Node::StaticClass());
    classes.Sort([](const Class& InA, const Class& InB) { return InA.Name < InB.Name; });

    Array<Class> spawnable;
    for (const Class& nodeClass : classes) {
        if (nodeClass.IsSubclassOf(Component::StaticClass())) {
            continue;
        }
        Object* probe = Object::Create(nodeClass);
        if (!probe) {
            continue;
        }
        delete probe;
        spawnable.Add(nodeClass);
    }
    return spawnable;
}

void AnimationEditorTab::SetPlaceholderClass(const Class& InClass) {
    Node* instance = m_Instance.Get();
    if (!instance || InClass == GetPlaceholderClass()) {
        return;
    }

    SharedObjectPtr<NodeRecord> state = NodeRecord::Capture(*instance);
    state->ClassName = InClass.Name;
    state->Inherited = false;
    BuildInstanceFrom(*state);
}

Node* AnimationEditorTab::GetAnimationRoot() const {
    Node* root = m_AnimationRoot.Get();
    return root ? root : m_Instance.Get();
}

void AnimationEditorTab::SetAnimationRoot(Node* InNode) {
    Node* instance = m_Instance.Get();
    Animation* animation = m_Animation.Get();
    if (!instance || !animation || !InNode) {
        return;
    }

    Array<int32_t> path;
    if (!Animation::MakePath(*instance, *InNode, path)) {
        AE_WARN("'{0}' is not part of the placeholder hierarchy", InNode->GetName());
        return;
    }

    if (!animation->GetKeys().IsEmpty() && InNode != GetAnimationRoot()) {
        AE_WARN("'{0}' is now the animated root; the {1} existing keys keep the paths they were recorded with",
                InNode->GetName(), animation->GetKeys().Size());
    }

    m_AnimationRoot = InNode;
    animation->SetRootPath(path);
}

bool AnimationEditorTab::IsAnimationRoot(Node* InNode) const {
    return InNode && InNode == GetAnimationRoot();
}

void AnimationEditorTab::CapturePose() {
    m_PreviewPose.Clear();
    Array<Node*> pending;
    if (Node* instance = m_Instance.Get()) {
        pending.Add(instance);
    }
    while (!pending.IsEmpty()) {
        Node* node = pending.LastItem();
        pending.RemoveLastItem();

        AnimationPose pose;
        pose.Target = node;
        pose.Values = JsonSerializer::SerializeObject(node);
        m_PreviewPose.Add(pose);

        for (uint32_t i = 0; i < node->GetChildCount(); i++) {
            pending.Add(node->GetChild((int)i));
        }
    }
}

void AnimationEditorTab::RestorePose() {
    for (const AnimationPose& pose : m_PreviewPose) {
        if (Node* node = pose.Target.Get()) {
            JsonSerializer::DeserializeObject(node, pose.Values);
        }
    }
    m_PreviewPose.Clear();
}

void AnimationEditorTab::SetPreviewing(bool InPreviewing) {
    if (m_Previewing == InPreviewing) {
        return;
    }
    m_Previewing = InPreviewing;

    if (InPreviewing) {
        CapturePose();
        return;
    }
    m_Recording = false;
    m_Playing = false;
    RestorePose();
}

void AnimationEditorTab::SetRecording(bool InRecording) {
    if (InRecording) {
        SetPreviewing(true);
    }
    m_Recording = InRecording;
    m_Playing = false;
}

void AnimationEditorTab::SetPlaying(bool InPlaying) {
    if (InPlaying) {
        SetPreviewing(true);
        m_Recording = false;
    }
    m_Playing = InPlaying;
}

int32_t AnimationEditorTab::GetFrame() const {
    return (int32_t)std::lround(m_FramePosition);
}

void AnimationEditorTab::SetFrame(int32_t InFrame) {
    Animation* animation = m_Animation.Get();
    const int32_t last = animation ? animation->GetFrameCount() : 0;
    m_FramePosition = (float)(InFrame < 0 ? 0 : (InFrame > last ? last : InFrame));
}

void AnimationEditorTab::SeedTrackStart(Node& InNode, const String& InPropertyName, const Array<int32_t>& InPath) {
    Animation* animation = m_Animation.Get();
    if (!animation || GetFrame() <= 0 || animation->FindTrack(InPath, InPropertyName) >= 0) {
        return;
    }

    for (const AnimationPose& pose : m_PreviewPose) {
        if (pose.Target.Get() != &InNode) {
            continue;
        }
        const nlohmann::json values = nlohmann::json::parse(pose.Values, nullptr, false);
        if (!values.is_discarded() && values.contains(InPropertyName)) {
            animation->SetKey(InPath, InPropertyName, 0, values[InPropertyName].dump());
        }
        return;
    }
}

bool AnimationEditorTab::KeyProperty(Node& InNode, const String& InPropertyName) {
    Animation* animation = m_Animation.Get();
    Node* root = GetAnimationRoot();
    Array<int32_t> path;
    if (!animation || !root || !Animation::MakePath(*root, InNode, path)) {
        return false;
    }

    SeedTrackStart(InNode, InPropertyName, path);
    return animation->KeyProperty(*root, InNode, InPropertyName, GetFrame());
}

void AnimationEditorTab::OnPropertyEdited(Object* InObject, const String& InPropertyName) {
    Node* node = Cast<Node>(InObject);
    // Editing anything outside the animated subtree is normal in this tab; it is simply not keyed.
    if (m_Recording && node) {
        KeyProperty(*node, InPropertyName);
    }
}

void AnimationEditorTab::ApplyCurrentFrame() {
    Animation* animation = m_Animation.Get();
    Node* root = GetAnimationRoot();
    if (m_Previewing && animation && root) {
        animation->ApplyAtFrame(root, m_FramePosition);
    }
}

void AnimationEditorTab::OnUIUpdate(const UIFrameContext& InContext) {
    Super::OnUIUpdate(InContext);

    Animation* animation = m_Animation.Get();
    if (!animation) {
        return;
    }

    const float last = (float)animation->GetFrameCount();
    if (m_FramePosition > last) {
        m_FramePosition = last;
    }
    if (!m_Previewing) {
        return;
    }

    if (m_Playing) {
        m_FramePosition += InContext.DeltaTime * animation->GetFrameRate();
        if (m_FramePosition >= last) {
            m_FramePosition = animation->IsLooping() ? std::fmod(m_FramePosition, last) : last;
            m_Playing = animation->IsLooping();
        }
    }
    ApplyCurrentFrame();
}

void AnimationEditorTab::Save() {
    Animation* animation = m_Animation.Get();
    Node* instance = m_Instance.Get();
    if (!animation || !instance) {
        AE_WARN("There is no animation open to save");
        return;
    }

    Array<int32_t> path;
    if (Node* root = m_AnimationRoot.Get()) {
        Animation::MakePath(*instance, *root, path);
    }
    animation->SetRootPath(path);

    // The placeholder is saved the way it was authored, never in whatever pose preview left it in.
    const bool wasRecording = m_Recording;
    if (m_Previewing) {
        RestorePose();
    }
    animation->CaptureFrom(*instance);
    if (NodeRecord* record = animation->GetRoot()) {
        record->Inherited = false;
    }
    if (m_Previewing) {
        CapturePose();
        m_Recording = wasRecording;
        ApplyCurrentFrame();
    }

    if (AssetManager::Get().SaveAsset(animation)) {
        BroadcastAssetSaved(animation, this);
    }
}

Asset* AnimationEditorTab::GetEditedAsset() const {
    return m_Animation.Get();
}

Node* AnimationEditorTab::GetAssetRootNode() const {
    return m_Instance.Get();
}

void AnimationEditorTab::OnAssetSaved(Asset* InAsset) {
    if (InAsset && InAsset == (Asset*)m_Animation.Get()) {
        OpenAnimation(m_Animation.Get());
    }
}

String AnimationEditorTab::GetTabTitle() const {
    Animation* animation = m_Animation.Get();
    return animation ? animation->GetDisplayName() : String("Untitled Animation");
}

VectorImage* AnimationEditorTab::GetTabIcon() const {
    return EditorIcons::Animation();
}

void AnimationEditorTab::BuildToolBar(UINode& InToolBar) {
    UIButton& save = UI::Button(InToolBar, "Save", [this] { Save(); });
    save.Size = { 70.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(save);

    UILabel* caption = InToolBar.Add<UILabel>();
    caption->Size = { 76.0_px, 1.0_rel };
    caption->FontSize = EditorStyle::FontSize;
    caption->Color = EditorStyle::TextDim;
    caption->HAlign = UIHAlign::Right;
    caption->VAlign = UIVAlign::Middle;
    caption->Text = "Placeholder";

    auto options = std::make_shared<Array<Class>>();
    UIDropdown* placeholder = InToolBar.Add<UIDropdown>();
    placeholder->Size = { 200.0_px, 1.0_rel };
    placeholder->SearchPlaceholder = "Search classes";
    placeholder->GetSelectedLabel = [this] { return GetPlaceholderClass().GetDisplayName(); };
    placeholder->GetOptions = [this, options] {
        *options = GetSelectablePlaceholderClasses();
        Array<UIDropdownOption> labels;
        for (const Class& nodeClass : *options) {
            UIDropdownOption option(nodeClass.GetDisplayName());
            option.Icon = EditorIcons::GetNodeIcon(nodeClass);
            option.IconTint = EditorStyle::Text;
            labels.Add(option);
        }
        return labels;
    };
    placeholder->GetSelectedIndex = [this, options] { return options->IndexOf(GetPlaceholderClass()); };
    placeholder->SelectionChanged = [this, options](int32_t InIndex) {
        if (InIndex >= 0 && InIndex < options->Size()) {
            SetPlaceholderClass((*options)[InIndex]);
        }
    };

    UIQuad* divider = InToolBar.Add<UIQuad>();
    divider->Size = { 1.0_px, 1.0_rel };
    divider->Color = EditorStyle::Border;

    UIButton* record = &EditorStyle::IconButton(InToolBar, EditorIcons::Record(), s_RecordColor, "Record", 92.0f,
                                                [this] { SetRecording(!IsRecording()); });
    record->Bind = [this, record] {
        record->NormalColor = IsRecording() ? s_RecordColor : EditorStyle::Button;
        record->HoverColor = IsRecording() ? s_RecordColor : EditorStyle::ButtonHover;
    };

    UIButton* loop = &UI::Button(InToolBar, "Loop", [this] {
        if (Animation* animation = m_Animation.Get()) {
            animation->SetLooping(!animation->IsLooping());
        }
    });
    loop->Size = { 70.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(*loop);
    loop->Bind = [this, loop] {
        Animation* animation = m_Animation.Get();
        const bool looping = animation && animation->IsLooping();
        loop->NormalColor = looping ? EditorStyle::Accent : EditorStyle::Button;
        loop->HoverColor = looping ? EditorStyle::AccentBright : EditorStyle::ButtonHover;
    };
}
