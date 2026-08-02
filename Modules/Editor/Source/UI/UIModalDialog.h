#pragma once
#include "GameFramework/UINode.h"
#include "Common/String.h"
#include <functional>
#include "UIModalDialog.gen.h"

class UIStack;

class UIModalDialog : public UINode {
public:
    ARTIFACT_CLASS();

    UIModalDialog();

    static UIModalDialog* Open(UINode& InOwner, const String& InTitle, const Vec2& InSize);

    UIStack* GetBody() const { return m_Body; }
    void AddButton(const String& InLabel, bool InPrimary, std::function<void()> InClicked);
    UINode* AddField(const String& InCaption, float InHeight = 24.0f);

    void Close();

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void Build(const String& InTitle, const Vec2& InSize);

    UINode* m_Panel = nullptr;
    UIStack* m_Body = nullptr;
    UIStack* m_Buttons = nullptr;
    bool m_Closing = false;
};
