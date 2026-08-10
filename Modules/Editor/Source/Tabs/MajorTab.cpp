#include "MajorTab.h"
#include "MinorTab.h"
#include "MinorTabStandaloneWindow.h"
#include "EditorWindow.h"
#include "EditorTransaction.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "Assets/Mesh.h"
#include "Assets/Blueprint.h"
#include "GameFramework/StaticMeshNode.h"

static constexpr float s_TransactionTimeout = 0.35f;
static constexpr int32_t s_HistoryDepth = 256;

static void AssignWorldToTabs(UIDockNode* InNode, World* InWorld) {
    if (!InNode) {
        return;
    }
    for (MinorTab* tab : InNode->GetTabs()) {
        tab->SetEditedWorld(InWorld);
    }
    AssignWorldToTabs(InNode->GetChildA(), InWorld);
    AssignWorldToTabs(InNode->GetChildB(), InWorld);
}

void MajorTab::SetEditedWorld(World* InWorld) {
    m_World = InWorld;
    AssignWorldToTabs(m_DockArea->GetRoot(), InWorld);
    for (WeakObjectPtr<MinorTabStandaloneWindow>& weak : m_FloatingWindows) {
        if (MinorTabStandaloneWindow* window = weak.Get()) {
            if (MinorTab* tab = window->GetTab()) {
                tab->SetEditedWorld(InWorld);
            }
        }
    }
}

MajorTab::MajorTab() {
    Fill();
    m_DockArea = Add<UIDockArea>();
}

VectorImage* MajorTab::GetTabIcon() const {
    return EditorIcons::Document();
}

void MajorTab::BroadcastAssetSaved(Asset* InAsset, MajorTab* InSource) {
    if (!InAsset) {
        return;
    }

    Array<MajorTab*> targets;
    for (const SharedObjectPtr<ThemedWindow>& window : ThemedWindow::GetAllWindows()) {
        EditorWindow* editorWindow = window.Get() ? window->As<EditorWindow>() : nullptr;
        if (!editorWindow) {
            continue;
        }
        for (MajorTab* tab : editorWindow->GetOpenTabs()) {
            if (tab && tab != InSource) {
                targets.Add(tab);
            }
        }
    }

    for (MajorTab* tab : targets) {
        tab->OnAssetSaved(InAsset);
    }
}

bool MajorTab::IsSpawnableAsset(Asset* InAsset) {
    return InAsset && (InAsset->IsA(Mesh::StaticClass()) || InAsset->IsA(Blueprint::StaticClass()));
}

Node* MajorTab::SpawnFromAsset(Asset* InAsset, Node& InParent) {
    if (Mesh* mesh = Cast<Mesh>(InAsset)) {
        StaticMeshNode* node = Cast<StaticMeshNode>(InParent.AttachChild(StaticMeshNode::StaticClass()));
        if (node) {
            node->SetMesh(mesh);
            node->SetName(mesh->GetDisplayName());
        }
        return node;
    }
    if (Blueprint* blueprint = Cast<Blueprint>(InAsset)) {
        Node* node = InParent.AttachChild(Class::FromBlueprint(blueprint->GetId()));
        if (node) {
            node->SetName(blueprint->GetDisplayName());
        }
        return node;
    }
    return nullptr;
}

void MajorTab::OnBind() {
    Super::OnBind();

    if (World* world = GetAuthoringWorld()) {
        world->ResolvePendingKills();
    }
}

void MajorTab::OnUIUpdate(const UIFrameContext& InContext) {
    Super::OnUIUpdate(InContext);

    if (m_OpenTransaction) {
        m_TransactionIdle += InContext.DeltaTime;
        if (m_TransactionIdle > s_TransactionTimeout) {
            EndTransaction();
        }
    }
}

MajorTab* MajorTab::FindFor(const UINode& InNode) {
    for (Node* current = const_cast<UINode*>(&InNode); current; current = current->GetParent()) {
        if (MajorTab* major = Cast<MajorTab>(current)) {
            return major;
        }
        if (MinorTab* minor = Cast<MinorTab>(current)) {
            return minor->GetMajorTab();
        }
    }
    return nullptr;
}

void MajorTab::BeginTransaction(const String& InTitle, Object* InObject) {
    if (!InObject) {
        return;
    }
    if (m_OpenTransaction && m_OpenTransaction->Title != InTitle) {
        EndTransaction();
    }
    if (!m_OpenTransaction) {
        m_OpenTransaction = SharedObjectPtr<EditorTransaction>(new EditorTransaction(InTitle));
    }
    m_OpenTransaction->Record(InObject);
    m_TransactionIdle = 0.0f;
}

void MajorTab::EndTransaction() {
    SharedObjectPtr<EditorTransaction> transaction = m_OpenTransaction;
    m_OpenTransaction = nullptr;
    m_TransactionIdle = 0.0f;
    if (!transaction || transaction->IsUnchanged()) {
        return;
    }

    m_UndoStack.Add(transaction);
    if (m_UndoStack.Size() > s_HistoryDepth) {
        m_UndoStack.RemoveFirstItem();
    }
    m_RedoStack.Clear();
}

String MajorTab::GetUndoTitle() const {
    if (m_OpenTransaction) {
        return m_OpenTransaction->Title;
    }
    return m_UndoStack.IsEmpty() ? String() : m_UndoStack[m_UndoStack.Last()]->Title;
}

String MajorTab::GetRedoTitle() const {
    return m_RedoStack.IsEmpty() ? String() : m_RedoStack[m_RedoStack.Last()]->Title;
}

void MajorTab::Undo() {
    EndTransaction();
    if (m_UndoStack.IsEmpty()) {
        return;
    }
    SharedObjectPtr<EditorTransaction> transaction = m_UndoStack.LastItem();
    m_UndoStack.RemoveLastItem();
    transaction->Restore(*this);
    m_RedoStack.Add(transaction);
}

void MajorTab::Redo() {
    EndTransaction();
    if (m_RedoStack.IsEmpty()) {
        return;
    }
    SharedObjectPtr<EditorTransaction> transaction = m_RedoStack.LastItem();
    m_RedoStack.RemoveLastItem();
    transaction->Restore(*this);
    m_UndoStack.Add(transaction);
}

Array<Object*> MajorTab::GetSelection() const {
    Array<Object*> alive;
    for (const WeakObjectPtr<Object>& weak : m_Selection) {
        if (Object* object = weak.Get()) {
            alive.Add(object);
        }
    }
    return alive;
}

int32_t MajorTab::GetSelectionCount() const {
    return GetSelection().Size();
}

Object* MajorTab::GetSoleSelection() const {
    Array<Object*> alive = GetSelection();
    return alive.Size() == 1 ? alive[0] : nullptr;
}

bool MajorTab::IsSelected(Object* InObject) const {
    if (!InObject) {
        return false;
    }
    for (const WeakObjectPtr<Object>& weak : m_Selection) {
        if (weak.Get() == InObject) {
            return true;
        }
    }
    return false;
}

void MajorTab::SetSelection(Object* InObject) {
    m_Selection.Clear();
    AddToSelection(InObject);
}

void MajorTab::SetSelection(const Array<Object*>& InObjects) {
    m_Selection.Clear();
    for (Object* object : InObjects) {
        AddToSelection(object);
    }
}

void MajorTab::AddToSelection(Object* InObject) {
    if (InObject && !IsSelected(InObject)) {
        m_Selection.Add(WeakObjectPtr<Object>(InObject));
    }
}

void MajorTab::RemoveFromSelection(Object* InObject) {
    for (int32_t i = 0; i < m_Selection.Size(); i++) {
        if (m_Selection[i].Get() == InObject) {
            m_Selection.RemoveAt(i);
            return;
        }
    }
}

void MajorTab::ToggleSelection(Object* InObject) {
    if (IsSelected(InObject)) {
        RemoveFromSelection(InObject);
    } else {
        AddToSelection(InObject);
    }
}

void MajorTab::ClearSelection() {
    m_Selection.Clear();
}

MajorTab::~MajorTab() {
    for (WeakObjectPtr<MinorTabStandaloneWindow>& weak : m_FloatingWindows) {
        if (MinorTabStandaloneWindow* window = weak.Get()) {
            ThemedWindow::DestroyWindow(window);
        }
    }
}

void MajorTab::FloatTab(MinorTab* InTab, const Vec2& InScreenPos) {
    SharedObjectPtr<MinorTabStandaloneWindow> window = MinorTabStandaloneWindow::Create(InTab, this, InScreenPos);
    m_FloatingWindows.Add(WeakObjectPtr<MinorTabStandaloneWindow>(window.Get()));
}

void MajorTab::ReDockFloatingTab(MinorTabStandaloneWindow* InWindow) {
    for (int32_t i = 0; i < m_FloatingWindows.Size(); i++) {
        if (m_FloatingWindows[i].Get() == InWindow) {
            m_FloatingWindows.RemoveAt(i);
            break;
        }
    }
    if (MinorTab* tab = InWindow->GetTab()) {
        m_DockArea->Dock(tab, UIDockSlot::Center);
    }
}

void MajorTab::SetFloatingWindowsVisible(bool InVisible) {
    for (WeakObjectPtr<MinorTabStandaloneWindow>& weak : m_FloatingWindows) {
        if (MinorTabStandaloneWindow* window = weak.Get()) {
            if (InVisible) {
                window->Show();
            } else {
                window->Hide();
            }
        }
    }
}
