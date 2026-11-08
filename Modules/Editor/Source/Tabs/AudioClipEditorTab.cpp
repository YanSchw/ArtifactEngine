#include "AudioClipEditorTab.h"
#include "AudioWaveformTab.h"
#include "DetailsTab.h"
#include "EditorWindow.h"
#include "Audio/AudioPreview.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "Assets/AssetManager.h"
#include "Assets/AudioClip.h"

AudioClipEditorTab::AudioClipEditorTab() {
    UIDockArea* area = GetDockArea();
    m_Waveform = area->DockNew<AudioWaveformTab>(UIDockSlot::Center);
    m_Waveform->Owner = this;
    area->DockNew<DetailsTab>(UIDockSlot::Right, nullptr, 0.35f);
}

AudioClipEditorTab::~AudioClipEditorTab() {
    AudioPreview::Stop(m_Clip.Get());
}

void AudioClipEditorTab::OpenClip(AudioClip* InClip) {
    m_Clip = InClip;

    AssetManager::Get().LoadAsset(InClip);
    SetSelection(InClip);

    if (EditorWindow* window = GetOwnerWindow()) {
        window->MarkChromeDirty();
    }
}

void AudioClipEditorTab::Save() {
    AudioClip* clip = m_Clip.Get();
    if (!clip) {
        AE_WARN("There is no audio clip open to save");
        return;
    }
    if (AssetManager::Get().SaveAsset(clip)) {
        BroadcastAssetSaved(clip, this);
    }
}

void AudioClipEditorTab::TogglePlayback() {
    AudioPreview::Toggle(m_Clip.Get(), m_Loop);
}

void AudioClipEditorTab::StopPlayback() {
    AudioPreview::Stop(m_Clip.Get());
}

void AudioClipEditorTab::SeekTo(double InTime) {
    AudioPreview::Seek(m_Clip.Get(), InTime, m_Loop);
}

void AudioClipEditorTab::SetLooping(bool InLoop) {
    m_Loop = InLoop;
    AudioPreview::SetLooping(m_Clip.Get(), InLoop);
}

void AudioClipEditorTab::Reimport() {
    m_ReimportPending = false;

    AudioClip* clip = m_Clip.Get();
    if (!clip) {
        return;
    }
    AudioPreview::Stop(clip);
    clip->Reimport();
}

void AudioClipEditorTab::OnObjectEdited(Object* InObject) {
    if (InObject == (Object*)m_Clip.Get()) {
        m_ReimportPending = true;
    }
}

void AudioClipEditorTab::OnUIUpdate(const UIFrameContext& InContext) {
    if (m_ReimportPending) {
        Reimport();
    }
    MajorTab::OnUIUpdate(InContext);
}

Asset* AudioClipEditorTab::GetEditedAsset() const {
    return m_Clip.Get();
}

String AudioClipEditorTab::GetTabTitle() const {
    AudioClip* clip = m_Clip.Get();
    return clip ? clip->GetDisplayName() : String("Untitled Audio Clip");
}

VectorImage* AudioClipEditorTab::GetTabIcon() const {
    return EditorIcons::Audio();
}

void AudioClipEditorTab::BuildToolBar(UINode& InToolBar) {
    EditorStyle::ToolButton(InToolBar, EditorIcons::Save(), "Save", [this] { Save(); });

    UIButton& reimport = UI::Button(InToolBar, "Reimport", [this] { m_ReimportPending = true; });
    reimport.Size = { 90.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(reimport);

    UIQuad* divider = InToolBar.Add<UIQuad>();
    divider->Size = { 1.0_px, 1.0_rel };
    divider->Color = EditorStyle::Border;

    UIButton& play = EditorStyle::ToolButton(InToolBar, EditorIcons::Play(), "Play / Pause (Space)", [this] { TogglePlayback(); });
    UISvg* playIcon = play.GetChildByClass(UISvg::StaticClass())->As<UISvg>();
    play.Bind = [this, playIcon] {
        const bool playing = AudioPreview::IsPlaying(m_Clip.Get());
        playIcon->Image = playing ? EditorIcons::Pause() : EditorIcons::Play();
        playIcon->Tint = playing ? EditorStyle::Text : EditorStyle::TransformY;
    };

    UIButton& stop = EditorStyle::ToolButton(InToolBar, EditorIcons::Stop(), "Stop", [this] { StopPlayback(); });
    UISvg* stopIcon = stop.GetChildByClass(UISvg::StaticClass())->As<UISvg>();
    stop.Bind = [this, stopIcon] {
        stopIcon->Tint = AudioPreview::IsActive(m_Clip.Get()) ? EditorStyle::TransformX : EditorStyle::TextDim;
    };

    UIButton& loop = EditorStyle::ToolButton(InToolBar, EditorIcons::Loop(), "Loop", [this] { SetLooping(!m_Loop); });
    UIButton* loopButton = &loop;
    loop.Bind = [this, loopButton] {
        EditorStyle::SetToggled(*loopButton, m_Loop);
        loopButton->HoverColor = m_Loop ? EditorStyle::AccentBright : EditorStyle::ButtonHover;
    };
}