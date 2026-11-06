#include "AudioWaveformTab.h"
#include "AudioClipEditorTab.h"
#include "Audio/AudioPreview.h"
#include "UI/UIAudioWaveform.h"
#include "UI/UIRoundedQuad.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UILabel.h"
#include "Assets/AudioClip.h"
#include "InputSystem/KeyboardDevice.h"
#include <cmath>
#include <cstdio>

static String FormatTime(double InSeconds) {
    const double clamped = std::max(InSeconds, 0.0);
    const int minutes = (int)(clamped / 60.0);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%d:%06.3f", minutes, clamped - minutes * 60.0);
    return buffer;
}

static String FormatChannels(uint32_t InChannels) {
    if (InChannels == 1) {
        return "Mono";
    }
    if (InChannels == 2) {
        return "Stereo";
    }
    return std::to_string(InChannels) + " channels";
}

static String FormatSize(size_t InBytes) {
    char buffer[32];
    if (InBytes >= 1024 * 1024) {
        std::snprintf(buffer, sizeof(buffer), "%.2f MB", (double)InBytes / (1024.0 * 1024.0));
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.1f KB", (double)InBytes / 1024.0);
    }
    return buffer;
}

static String DescribeSamples(const AudioSampleBuffer& InSamples) {
    const AudioFormat& format = InSamples.GetFormat();
    const String encoding = format.SampleFormat == AudioSampleFormat::Int16 ? "16-bit PCM" : "32-bit float";
    return std::to_string(format.SampleRate) + " Hz   " + FormatChannels(format.Channels) + "   " + encoding + "   "
         + std::to_string(format.GetBitrate() / 1000) + " kbps   " + FormatSize(InSamples.GetData().Size());
}

AudioWaveformTab::AudioWaveformTab() {
    UIVStack* column = Add<UIVStack>();
    column->Fill();
    column->Padding = UIPadding(10.0f, 8.0f);
    column->Gap = 6.0f;

    UINode* header = column->Add<UINode>();
    header->Size = { 1.0_rel, 22.0_px };

    UILabel* info = header->Add<UILabel>();
    info->Fill();
    info->FontSize = EditorStyle::FontSize - 1.0f;
    info->Color = EditorStyle::TextDim;
    info->VAlign = UIVAlign::Middle;
    info->Bind = [this, info] {
        AudioClip* clip = Owner ? Owner->GetClip() : nullptr;
        const SharedAudioSamples samples = clip ? clip->GetSamples() : nullptr;
        info->Text = samples ? DescribeSamples(*samples) : String("No audio data");
    };

    UILabel* time = header->Add<UILabel>();
    time->Fill();
    time->FontSize = EditorStyle::FontSize;
    time->Color = EditorStyle::Text;
    time->HAlign = UIHAlign::Right;
    time->VAlign = UIVAlign::Middle;
    time->Bind = [this, time] {
        AudioClip* clip = Owner ? Owner->GetClip() : nullptr;
        const double duration = clip ? clip->GetDuration() : 0.0;
        time->Text = FormatTime(AudioPreview::GetTime(clip)) + " / " + FormatTime(duration);
    };

    UIRoundedQuad* panel = column->Add<UIRoundedQuad>();
    panel->Size = { 1.0_rel, 1.0_rel };
    panel->Color = EditorStyle::PanelDark;
    panel->CornerRadius = Vec4(4.0f);

    m_Waveform = panel->Add<UIAudioWaveform>();
    m_Waveform->Fill();
    m_Waveform->Padding = UIPadding(0.0f, 10.0f);
    m_Waveform->Interactable = true;
    m_Waveform->Cursor = CursorIcon::ResizeH;
    m_Waveform->WaveColor = EditorIcons::GetAssetColor(AudioClip::StaticClass());
    m_Waveform->PlayedColor = HexColor(0xC4C5FF);
    m_Waveform->PlayheadColor = EditorStyle::TextBright;
    m_Waveform->CenterLineColor = Vec4(1.0f, 1.0f, 1.0f, 0.06f);
    m_Waveform->Scrubbed = [this](double InTime) {
        if (Owner) {
            Owner->SeekTo(InTime);
        }
    };
    m_Waveform->Bind = [this] {
        AudioClip* clip = Owner ? Owner->GetClip() : nullptr;
        m_Waveform->Samples = clip ? clip->GetSamples() : nullptr;
        m_Waveform->PlayheadTime = AudioPreview::IsActive(clip) ? AudioPreview::GetTime(clip) : -1.0;
    };
}

VectorImage* AudioWaveformTab::GetTabIcon() const {
    return EditorIcons::Audio();
}

void AudioWaveformTab::OnUIUpdate(const UIFrameContext& InContext) {
    MinorTab::OnUIUpdate(InContext);

    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (Owner && keyboard && AcceptsShortcuts() && keyboard->IsDown(KeyCode::Space)) {
        Owner->TogglePlayback();
    }
}
