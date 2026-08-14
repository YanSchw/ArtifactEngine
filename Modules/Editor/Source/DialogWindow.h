#pragma once
#include "ThemedWindow.h"
#include <functional>
#include "DialogWindow.gen.h"

class UIButton;
class UILabel;
class UIStack;
class UITextArea;

class DialogWindow : public ThemedWindow {
public:
    ARTIFACT_CLASS();

protected:
    static constexpr float FieldHeight = 26.0f;

    DialogWindow(const WindowParams& InParams);

    static WindowParams MakeParams(const String& InTitle, uint32_t InWidth, uint32_t InContentHeight);
    /** The window the dialog is opening from. Resolve it before constructing the dialog, because
     *  the new window takes the focus away from it. */
    static Window* CurrentOpener();
    /** Centers the built dialog over its opener and hands it to the window registry. */
    static void Present(const SharedObjectPtr<DialogWindow>& InWindow, Window* InOpener);

    /** Background, body column and footer row. Everything else is added after it. */
    void BuildFrame();

    UILabel* AddCaption(const String& InText);
    UITextArea* AddTextField(const String& InPlaceholder);
    /** A [caption][field] row. */
    UITextArea* AddNamedTextField(const String& InCaption);
    /** A sunken panel taking whatever height the body has left over. */
    UINode* AddFillPanel();
    UIButton* AddFooterButton(const String& InCaption, bool InPrimary, std::function<void()> InClicked);

    /** Strips whatever would turn a typed name into a path or an invalid file name. */
    static String SanitizeName(const String& InName);

    virtual void PreUIRender(double InDeltaTime) override;

private:
    UIStack* m_Body = nullptr;
    UIStack* m_Footer = nullptr;
};
