#include "InputMappingEditorTab.h"

#include "InputActionListTab.h"
#include "DetailsTab.h"
#include "EditorWindow.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "Assets/AssetManager.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/InputActionMapping.h"
#include "InputSystem/InputAction.h"
#include "InputSystem/InputBinding.h"

template<typename T>
static void RemoveItem(Array<SharedObjectPtr<T>>& InItems, const Object* InItem) {
    for (int32_t i = 0; i < InItems.Size(); i++) {
        if ((const Object*)InItems[i].Get() == InItem) {
            InItems.RemoveAt(i);
            return;
        }
    }
}

/** "Keyboard/W", "Keyboard/S" reads as "Keyboard: W / S" when every part shares one device. */
static String DescribeParts(const Array<String>& InPaths) {
    String device;
    bool bound = false;
    bool sharedDevice = true;
    for (const String& path : InPaths) {
        if (path.empty()) {
            continue;
        }
        const size_t slash = path.find('/');
        const String pathDevice = slash == String::npos ? String() : path.substr(0, slash);
        sharedDevice = sharedDevice && !pathDevice.empty() && (!bound || pathDevice == device);
        device = pathDevice;
        bound = true;
    }
    if (!bound) {
        return "Unbound";
    }

    String text;
    for (const String& path : InPaths) {
        if (!text.empty()) {
            text += " / ";
        }
        if (path.empty()) {
            text += "-";
        } else {
            text += sharedDevice ? path.substr(device.size() + 1) : path;
        }
    }
    return sharedDevice ? device + ": " + text : text;
}

InputMappingEditorTab::InputMappingEditorTab() {
    UIDockArea* area = GetDockArea();
    area->DockNew<InputActionListTab>(UIDockSlot::Center);
    area->DockNew<DetailsTab>(UIDockSlot::Right, nullptr, 0.5f);
}

void InputMappingEditorTab::OpenMapping(InputActionMapping* InMapping) {
    m_Mapping = InMapping;

    AssetManager::Get().LoadAsset(InMapping);
    SetSelection(InMapping);

    if (EditorWindow* window = GetOwnerWindow()) {
        window->MarkChromeDirty();
    }
}

void InputMappingEditorTab::Save() {
    InputActionMapping* mapping = GetMapping();
    if (!mapping) {
        AE_WARN("There is no input mapping open to save");
        return;
    }
    if (AssetManager::Get().SaveAsset(mapping)) {
        BroadcastAssetSaved(mapping, this);
    }
}

InputAction* InputMappingEditorTab::AddAction() {
    InputActionMapping* mapping = GetMapping();
    if (!mapping) {
        return nullptr;
    }

    InputAction* action = Object::Create<InputAction>();
    action->Name = MakeUniqueActionName();

    BeginTransaction("Add Action", mapping);
    mapping->Actions.Add(action);
    EndTransaction();

    SetSelection(action);
    return action;
}

InputBinding* InputMappingEditorTab::AddBinding(InputAction* InAction, const Class& InBindingClass) {
    InputActionMapping* mapping = GetMapping();
    if (!mapping || !InAction) {
        return nullptr;
    }

    InputBinding* binding = Cast<InputBinding>(Object::Create(InBindingClass));
    if (!binding) {
        AE_ERROR("Cannot create an input binding of class '{0}'", InBindingClass.Name);
        return nullptr;
    }

    BeginTransaction("Add Binding", mapping);
    InAction->Bindings.Add(binding);
    EndTransaction();

    SetSelection(binding);
    return binding;
}

void InputMappingEditorTab::Delete(Object* InActionOrBinding) {
    InputActionMapping* mapping = GetMapping();
    if (!mapping) {
        return;
    }

    if (InputBinding* binding = Cast<InputBinding>(InActionOrBinding)) {
        InputAction* action = FindOwningAction(binding);
        if (!action) {
            return;
        }
        BeginTransaction("Delete Binding", mapping);
        RemoveItem(action->Bindings, binding);
        EndTransaction();
        SetSelection(action);
    } else if (InputAction* action = Cast<InputAction>(InActionOrBinding)) {
        BeginTransaction("Delete Action", mapping);
        RemoveItem(mapping->Actions, action);
        EndTransaction();
        SetSelection(mapping);
    }
}

InputAction* InputMappingEditorTab::FindOwningAction(const InputBinding* InBinding) const {
    InputActionMapping* mapping = GetMapping();
    if (!mapping || !InBinding) {
        return nullptr;
    }
    for (const SharedObjectPtr<InputAction>& action : mapping->Actions) {
        for (const SharedObjectPtr<InputBinding>& binding : action->Bindings) {
            if (binding.Get() == InBinding) {
                return action.Get();
            }
        }
    }
    return nullptr;
}

Array<String> InputMappingEditorTab::GetControlPathsFor(const InputBinding* InBinding) const {
    InputSystem& input = InputSystem::Get();
    if (InBinding && InBinding->IsA<PathBinding>()) {
        const InputAction* action = FindOwningAction(InBinding);
        return input.GetControlPaths(action ? action->Type : InputValueType::Bool);
    }
    return input.GetControlPaths(InputValueType::Bool) + input.GetControlPaths(InputValueType::Float);
}

Array<Class> InputMappingEditorTab::GetBindingClassesFor(InputValueType InType) {
    Array<Class> classes = { PathBinding::StaticClass() };
    if (InType == InputValueType::Float) {
        classes.Add(Axis1DComposite::StaticClass());
    } else if (InType == InputValueType::Vec2) {
        classes.Add(Vector2Composite::StaticClass());
    }
    return classes;
}

String InputMappingEditorTab::GetBindingLabel(const Class& InBindingClass) {
    if (InBindingClass == PathBinding::StaticClass()) {
        return "Path";
    }
    if (InBindingClass == Axis1DComposite::StaticClass()) {
        return "1D Axis";
    }
    if (InBindingClass == Vector2Composite::StaticClass()) {
        return "2D Vector";
    }
    return InBindingClass.GetDisplayName();
}

String InputMappingEditorTab::DescribeBinding(const InputBinding& InBinding) {
    if (const PathBinding* path = InBinding.As<PathBinding>()) {
        return path->Path.empty() ? String("Unbound") : path->Path;
    }
    if (const Axis1DComposite* axis = InBinding.As<Axis1DComposite>()) {
        return DescribeParts({ axis->Negative, axis->Positive });
    }
    if (const Vector2Composite* vector = InBinding.As<Vector2Composite>()) {
        return DescribeParts({ vector->Up, vector->Down, vector->Left, vector->Right });
    }
    return GetBindingLabel(InBinding.GetClass());
}

InputMappingEditorTab::ItemPath InputMappingEditorTab::FindPath(Object* InObject) const {
    InputActionMapping* mapping = GetMapping();
    if (!mapping || !InObject) {
        return ItemPath();
    }
    for (int32_t a = 0; a < mapping->Actions.Size(); a++) {
        InputAction* action = mapping->Actions[a].Get();
        if (action == InObject) {
            return { a, -1 };
        }
        for (int32_t b = 0; b < action->Bindings.Size(); b++) {
            if (action->Bindings[b].Get() == InObject) {
                return { a, b };
            }
        }
    }
    return ItemPath();
}

Object* InputMappingEditorTab::Resolve(const ItemPath& InPath) const {
    InputActionMapping* mapping = GetMapping();
    if (!mapping || InPath.Action < 0 || InPath.Action >= mapping->Actions.Size()) {
        return mapping;
    }
    InputAction* action = mapping->Actions[InPath.Action].Get();
    if (InPath.Binding < 0 || InPath.Binding >= action->Bindings.Size()) {
        return action;
    }
    return action->Bindings[InPath.Binding].Get();
}

String InputMappingEditorTab::MakeUniqueActionName() const {
    String name = "NewAction";
    for (int32_t suffix = 1; GetMapping()->Find(name); suffix++) {
        name = "NewAction" + std::to_string(suffix);
    }
    return name;
}

void InputMappingEditorTab::OnObjectEdited(Object* InObject) {
    // Undo and redo replace every action and binding, so the selection is found again by position.
    if (InObject == (Object*)GetMapping() && GetSelection().IsEmpty()) {
        SetSelection(Resolve(m_SelectedPath));
    }
}

void InputMappingEditorTab::OnUIUpdate(const UIFrameContext& InContext) {
    MajorTab::OnUIUpdate(InContext);
    m_SelectedPath = FindPath(GetSoleSelection());
}

Asset* InputMappingEditorTab::GetEditedAsset() const {
    return m_Mapping.Get();
}

String InputMappingEditorTab::GetTabTitle() const {
    InputActionMapping* mapping = GetMapping();
    return mapping ? mapping->GetDisplayName() : String("Untitled Input Mapping");
}

VectorImage* InputMappingEditorTab::GetTabIcon() const {
    return EditorIcons::InputMapping();
}

void InputMappingEditorTab::BuildToolBar(UINode& InToolBar) {
    EditorStyle::ToolButton(InToolBar, EditorIcons::Save(), "Save", [this] { Save(); });
}
