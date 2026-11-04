#pragma once
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "InputActionListTab.gen.h"

class InputMappingEditorTab;
class InputAction;
class UIButton;
class UILabel;
class UIVStack;
class UIMenuModel;
class VectorImage;

/** Lists the actions of the open InputActionMapping, each followed by its bindings. */
class InputActionListTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    InputActionListTab();

    virtual String GetTabTitle() const override { return "Actions"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    InputMappingEditorTab* GetEditor() const;
    InputAction* GetSelectedAction() const;
    Array<Object*> CollectRows() const;

    void Refresh();
    void RebuildRows(const Array<Object*>& InRows);
    void AddRow(Object* InTarget);

    void BuildRowMenu(UIMenuModel& OutMenu, Object* InTarget);
    void BuildAddBindingMenu(UIMenuModel& OutMenu, InputAction* InAction);

    UIButton* m_AddBindingButton = nullptr;
    UIVStack* m_List = nullptr;
    UILabel* m_EmptyLabel = nullptr;
    Array<WeakObjectPtr<Object>> m_Rows;
};
