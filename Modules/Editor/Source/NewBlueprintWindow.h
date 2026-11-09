#pragma once
#include "DialogWindow.h"
#include "Object/Object.h"
#include "NewBlueprintWindow.gen.h"

class Asset;

class NewBlueprintWindow : public DialogWindow {
public:
    ARTIFACT_CLASS();
protected:
    NewBlueprintWindow(const WindowParams& InParams);
public:
    using CreatedFn = std::function<void(Asset*)>;

    static NewBlueprintWindow* Open(const String& InDirectory, const String& InName, CreatedFn InCreated);

private:
    void BuildContent();
    void Create();

    String m_Directory;
    String m_Name;
    Class m_ParentClass;
    CreatedFn m_Created;
    class UIClassPicker* m_ClassPicker = nullptr;
};
