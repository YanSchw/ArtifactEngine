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

protected:
    virtual void PreUIRender(double InDeltaTime) override;

private:
    void BuildContent();
    void BuildClassList();
    void Create();

    String m_Directory;
    String m_Name;
    String m_Filter;
    Class m_ParentClass;
    CreatedFn m_Created;
    UIStack* m_ClassList = nullptr;
    bool m_ListDirty = true;
};
