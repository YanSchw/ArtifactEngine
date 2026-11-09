#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "Particles/ParticleHandle.h"
#include "ParticleSystemEditorTab.gen.h"

class ParticleSystemAsset;
class ViewportTab;
class VectorImage;

class ParticleSystemEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    ParticleSystemEditorTab();

    void OpenSystem(ParticleSystemAsset* InSystem);
    ParticleSystemAsset* GetSystem() const { return m_System.Get(); }

    void Save();
    void TogglePlayback();
    void RestartPlayback();
    void StopPlayback();
    void SeekPlayback(float InTime);

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual void OnObjectEdited(Object* InObject) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    String GetStatsText() const;

    WeakObjectPtr<ParticleSystemAsset> m_System;
    ParticleHandle m_Preview;
    ViewportTab* m_Viewport = nullptr;
    float m_RestartDelay = 0.0f;
    float m_PlaybackSpeed = 1.0f;
    bool m_Looping = true;
};
