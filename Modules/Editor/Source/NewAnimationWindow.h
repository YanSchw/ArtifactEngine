#pragma once
#include "DialogWindow.h"
#include "Object/Object.h"
#include "NewAnimationWindow.gen.h"

class Asset;

class NewAnimationWindow : public DialogWindow {
public:
    ARTIFACT_CLASS();
protected:
    NewAnimationWindow(const WindowParams& InParams);
public:
    using CreatedFn = std::function<void(Asset*)>;

    static NewAnimationWindow* Open(const String& InDirectory, const String& InName, CreatedFn InCreated);

private:
    void BuildContent();
    void Create();

    String m_Directory;
    String m_Name;
    Class m_PlaceholderClass;
    CreatedFn m_Created;
    class UIClassPicker* m_ClassPicker = nullptr;
};
