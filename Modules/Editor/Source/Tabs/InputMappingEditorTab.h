#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "InputSystem/InputValue.h"
#include "InputMappingEditorTab.gen.h"

class InputActionMapping;
class InputAction;
class InputBinding;
class VectorImage;

/** Edits the actions of an InputActionMapping and the bindings that drive them. */
class InputMappingEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    InputMappingEditorTab();

    void OpenMapping(InputActionMapping* InMapping);
    InputActionMapping* GetMapping() const { return m_Mapping.Get(); }

    void Save();

    InputAction* AddAction();
    InputBinding* AddBinding(InputAction* InAction, const Class& InBindingClass);
    void Delete(Object* InActionOrBinding);

    InputAction* FindOwningAction(const InputBinding* InBinding) const;
    Array<String> GetControlPathsFor(const InputBinding* InBinding) const;

    static Array<Class> GetBindingClassesFor(InputValueType InType);
    static String GetBindingLabel(const Class& InBindingClass);
    static String DescribeBinding(const InputBinding& InBinding);

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual void OnObjectEdited(Object* InObject) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    struct ItemPath {
        int32_t Action = -1;
        int32_t Binding = -1;
    };

    ItemPath FindPath(Object* InObject) const;
    Object* Resolve(const ItemPath& InPath) const;
    String MakeUniqueActionName() const;

    WeakObjectPtr<InputActionMapping> m_Mapping;
    ItemPath m_SelectedPath;
};
