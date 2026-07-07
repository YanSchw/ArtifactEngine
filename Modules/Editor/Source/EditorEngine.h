#pragma once
#include "Core/Engine.h"
#include "EditorEngine.gen.h"

class UICanvas;
class UIRenderer;

class EditorEngine : public Engine {
public:
    ARTIFACT_CLASS();

protected:
    virtual void Initialize() override;
    virtual bool MainTick(double InDeltaTime) override;
    virtual void TickInput(double InDeltaTime) override;
    virtual void Shutdown() override;

private:
    void BuildDemoUI();

    UIRenderer* m_UIRenderer = nullptr;
    UICanvas* m_UICanvas = nullptr;
};