#pragma once
#include "Core/Engine.h"
#include "Object/Pointer.h"
#include "EditorEngine.gen.h"

class SceneEditorTab;

class EditorEngine : public Engine {
public:
    ARTIFACT_CLASS();

    static EditorEngine* Get();

    GameInstance* CreatePlayInstance(SceneEditorTab* InTab);
    void DisposePlayInstance();

protected:
    virtual void Initialize() override;
    virtual bool MainTick(double InDeltaTime) override;
    virtual void TickInput(double InDeltaTime) override;
    virtual void Shutdown() override;

private:
    void RenderFrame(double InDeltaTime);
    bool IsGameInputActive() const;
    void UpdatePlayCursor();

    WeakObjectPtr<SceneEditorTab> m_PlayingTab;
    bool m_GameCursorLocked = false;
    bool m_HadGameInput = false;
};
