#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "AudioClipEditorTab.gen.h"

class AudioClip;
class AudioWaveformTab;
class VectorImage;

class AudioClipEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    AudioClipEditorTab();
    virtual ~AudioClipEditorTab();

    void OpenClip(AudioClip* InClip);
    AudioClip* GetClip() const { return m_Clip.Get(); }

    void Save();
    void TogglePlayback();
    void StopPlayback();
    void SeekTo(double InTime);
    bool IsLooping() const { return m_Loop; }
    void SetLooping(bool InLoop);

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual void OnObjectEdited(Object* InObject) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void Reimport();

    WeakObjectPtr<AudioClip> m_Clip;
    AudioWaveformTab* m_Waveform = nullptr;
    bool m_Loop = false;
    bool m_ReimportPending = false;
};
