#pragma once
#include "MinorTab.h"
#include "AudioWaveformTab.gen.h"

class AudioClipEditorTab;
class UIAudioWaveform;
class VectorImage;

class AudioWaveformTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    AudioWaveformTab();

    AudioClipEditorTab* Owner = nullptr;

    virtual String GetTabTitle() const override { return "Waveform"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    UIAudioWaveform* m_Waveform = nullptr;
};
