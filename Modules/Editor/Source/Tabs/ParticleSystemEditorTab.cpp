#include "ParticleSystemEditorTab.h"
#include "DetailsTab.h"
#include "EditorWindow.h"
#include "ViewportTab.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/ParticleWorld.h"
#include "Assets/AssetManager.h"
#include "Assets/ParticleSystemAsset.h"
#include "Particles/ParticlePreview.h"
#include "Particles/ParticleSystemInstance.h"
#include "Settings/EditorSettings.h"
#include "UI/UIDragNumber.h"

static constexpr float s_RestartDelay = 0.6f;

ParticleSystemEditorTab::ParticleSystemEditorTab() {
    SetEditedWorld(new World());

    UIDockArea* area = GetDockArea();
    m_Viewport = area->DockNew<ViewportTab>(UIDockSlot::Center);
    area->DockNew<DetailsTab>(UIDockSlot::Right, nullptr, 0.38f);
}

void ParticleSystemEditorTab::OpenSystem(ParticleSystemAsset* InSystem) {
    m_System = InSystem;
    AssetManager::Get().LoadAsset(InSystem);

    ParticleWorld* particles = GetAuthoringWorld()->GetSubsystem<ParticleWorld>();
    particles->DestroyInstance(m_Preview);
    m_Preview = particles->CreateInstance(InSystem, Mat4(1.0f));
    m_Preview.SetSimulationSpeed(m_PlaybackSpeed);
    m_Preview.Play();
    m_RestartDelay = 0.0f;
    SetSelection(InSystem);

    if (EditorWindow* window = GetOwnerWindow()) {
        window->MarkChromeDirty();
    }
}

void ParticleSystemEditorTab::Save() {
    ParticleSystemAsset* system = m_System.Get();
    if (!system) {
        AE_WARN("There is no particle system open to save");
        return;
    }
    if (AssetManager::Get().SaveAsset(system)) {
        BroadcastAssetSaved(system, this);
    }
}

void ParticleSystemEditorTab::TogglePlayback() {
    if (m_Preview.IsPaused()) {
        m_Preview.SetPaused(false);
    } else if (m_Preview.IsAlive()) {
        m_Preview.SetPaused(true);
    } else {
        m_Preview.Play();
    }
    m_Looping = true;
}

void ParticleSystemEditorTab::RestartPlayback() {
    m_Preview.Restart();
    m_RestartDelay = 0.0f;
    m_Looping = true;
}

void ParticleSystemEditorTab::StopPlayback() {
    m_Preview.Stop(ParticleStopBehavior::StopEmittingAndClear);
    m_Looping = false;
}

void ParticleSystemEditorTab::SeekPlayback(float InTime) {
    m_Preview.Seek(InTime);
    m_Preview.SetPaused(true);
    m_Looping = true;
}

void ParticleSystemEditorTab::OnObjectEdited(Object* InObject) {
    if (ParticleSystemAsset* system = m_System.Get(); system && InObject == (Object*)system) {
        system->AcquireReferencedAssets();
    }
}

void ParticleSystemEditorTab::OnUIUpdate(const UIFrameContext& InContext) {
    ParticleSystemAsset* system = m_System.Get();
    if (system && GetSelectionCount() == 0) {
        SetSelection(system);
    }

    if (m_Looping && !m_Preview.IsAlive()) {
        m_RestartDelay += (float)InContext.DeltaTime;
        if (m_RestartDelay >= s_RestartDelay) {
            m_RestartDelay = 0.0f;
            m_Preview.Play();
        }
    }
    if (system && EditorSettings::Get().Gizmos.ShowGizmos) {
        ParticlePreview::DrawShapes(*GetAuthoringWorld(), *system, Mat4(1.0f));
    }
    MajorTab::OnUIUpdate(InContext);
}

String ParticleSystemEditorTab::GetStatsText() const {
    const ParticleSystemInstance* instance = m_Preview.Get();
    if (!instance) {
        return String();
    }
    return std::to_string(instance->GetParticleCount()) + " particles";
}

Asset* ParticleSystemEditorTab::GetEditedAsset() const {
    return m_System.Get();
}

String ParticleSystemEditorTab::GetTabTitle() const {
    ParticleSystemAsset* system = m_System.Get();
    return system ? system->GetDisplayName() : String("Untitled Particle System");
}

VectorImage* ParticleSystemEditorTab::GetTabIcon() const {
    return EditorIcons::Particles();
}

void ParticleSystemEditorTab::BuildToolBar(UINode& InToolBar) {
    EditorStyle::ToolButton(InToolBar, EditorIcons::Save(), "Save", [this] { Save(); });

    UIQuad* divider = InToolBar.Add<UIQuad>();
    divider->Size = { 1.0_px, 1.0_rel };
    divider->Color = EditorStyle::Border;

    UIButton& play = EditorStyle::ToolButton(InToolBar, EditorIcons::Play(), "Play / Pause", [this] { TogglePlayback(); });
    UISvg* playIcon = play.GetChildByClass(UISvg::StaticClass())->As<UISvg>();
    play.Bind = [this, playIcon] {
        const bool playing = m_Preview.IsPlaying();
        playIcon->Image = playing ? EditorIcons::Pause() : EditorIcons::Play();
        playIcon->Tint = playing ? EditorStyle::Text : EditorStyle::TransformY;
    };

    EditorStyle::ToolButton(InToolBar, EditorIcons::Reset(), "Restart", [this] { RestartPlayback(); });

    UIButton& stop = EditorStyle::ToolButton(InToolBar, EditorIcons::Stop(), "Stop", [this] { StopPlayback(); });
    UISvg* stopIcon = stop.GetChildByClass(UISvg::StaticClass())->As<UISvg>();
    stop.Bind = [this, stopIcon] {
        stopIcon->Tint = m_Preview.IsAlive() ? EditorStyle::TransformX : EditorStyle::TextDim;
    };

    const auto addNumber = [&InToolBar](const String& InCaption, std::function<double()> InGet, std::function<void(double)> InSet) {
        UILabel* caption = InToolBar.Add<UILabel>();
        caption->Size = { 48.0_px, 1.0_rel };
        caption->FontSize = EditorStyle::FontSize;
        caption->Color = EditorStyle::TextDim;
        caption->HAlign = UIHAlign::Right;
        caption->VAlign = UIVAlign::Middle;
        caption->Text = InCaption;

        UIDragNumber* number = InToolBar.Add<UIDragNumber>();
        number->Size = { 64.0_px, 1.0_rel };
        number->MinValue = 0.0;
        number->Get = std::move(InGet);
        number->Set = std::move(InSet);
    };
    addNumber("Speed", [this] { return (double)m_PlaybackSpeed; }, [this](double InValue) {
        m_PlaybackSpeed = (float)InValue;
        m_Preview.SetSimulationSpeed(m_PlaybackSpeed);
    });
    addNumber("Time", [this] {
        const ParticleSystemInstance* instance = m_Preview.Get();
        return instance ? (double)instance->GetPlaybackTime() : 0.0;
    }, [this](double InValue) { SeekPlayback((float)InValue); });

    UILabel* stats = InToolBar.Add<UILabel>();
    stats->Size = { 180.0_px, 1.0_rel };
    stats->FontSize = EditorStyle::FontSize;
    stats->Color = EditorStyle::TextDim;
    stats->VAlign = UIVAlign::Middle;
    stats->Bind = [this, stats] { stats->Text = GetStatsText(); };
}
