#pragma once
#include "DialogWindow.h"
#include "Object/Object.h"
#include "NewDataAssetWindow.gen.h"

class Asset;

class NewDataAssetWindow : public DialogWindow {
public:
    ARTIFACT_CLASS();
protected:
    NewDataAssetWindow(const WindowParams& InParams);
public:
    using CreatedFn = std::function<void(Asset*)>;

    static NewDataAssetWindow* Open(const String& InDirectory, const String& InName, CreatedFn InCreated);

private:
    void BuildContent();
    void Create();

    String m_Directory;
    String m_Name;
    CreatedFn m_Created;
    class UIClassPicker* m_ClassPicker = nullptr;
};
