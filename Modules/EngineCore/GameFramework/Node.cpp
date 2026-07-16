#include "Node.h"
#include "Component.h"
#include "Node3D.h"
#include "World.h"
#include "Core/Log.h"

Node::Node() {
    SetName(GetClass().Name);
}

Node::~Node() {
    // Unparent Children - Before actually destroying the Parent --- if the World exits
    if (GetWorld()) {
        while (HasChildren()) {
            GetChild(0)->ForceSetParent(GetParent(), true, false);
        }
    } else {
        // There is no World, the Node exits by itself and there is no gameplay context.
        // Manually delete this Nodes children...
        while (HasChildren()) {
            Node* child = GetChild(0);
            child->ForceSetParent(GetParent(), true, false);
            delete child;
        }
    }

    ForceSetParent(nullptr, true, false);

    if (GetWorld()) {
        GetWorld()->UnregisterNode(this);
    }
}

String Node::GetName() const {
    return m_Name;
}

static bool DoesNameExistInHierarchy(const String& InName, Node* InRoot, Node* InToBeIgnored) {
    if (!InRoot) return false;
    if (InRoot->GetName() == InName && InRoot != InToBeIgnored) return true;

    for (int i = 0; i < InRoot->GetChildCount(); i++) {
        if (DoesNameExistInHierarchy(InName, InRoot->GetChild(i), InToBeIgnored))
            return true;
    }

    return false;
}

static String IncrementNameIndex(const String& InName) {
    return InName + "|";
}

void Node::SetName(const String& InName) {
    if (InName.empty()) {
        SetName(GetClass().Name);
        return;
    }

    if (WasBeginPlayCalled()) {
        m_Name = InName;
    } else {
        // Duplicate names are not allowed, in a non-gameplay context!
        bool alreadyExists = DoesNameExistInHierarchy(InName, GetRootNode(), this);
        if (!alreadyExists) {
            m_Name = InName;
        } else {
            SetName(IncrementNameIndex(InName));
        }
    }
}

void Node::WorldUpdate(float InDeltatime) {
    AE_INFO("Node::WorldUpdate(float InDeltatime)[{0}]: {1}", GetName(), InDeltatime);
}

void Node::SetUpdateFlag(UpdateFlag InFlag) {
    m_UpdateFlags |= InFlag;

    if (!IsInitialized()) {
        return;
    }

    GetWorld()->ReregisterNode(this);
}

bool Node::IsUpdateFlagSet(UpdateFlag InFlag) const {
    return 0 != ((int32_t)m_UpdateFlags & (int32_t)InFlag);
}

bool Node::IsInitialized() const {
    return m_Initialized;
}

bool Node::IsEnabled() const {
    return m_EnabledInHierarchy;
}

bool Node::IsSelfEnabled() const {
    return m_Enabled;
}

void Node::SetEnabled(bool InEnabled) {
    m_Enabled = InEnabled;
    m_EnabledInHierarchy = InEnabled ? (GetParent() ? GetParent()->m_EnabledInHierarchy : InEnabled) : false;

    TickTransform();
}

bool Node::WasBeginPlayCalled() const {
    return m_WasBeginPlayCalled;
}

bool Node::IsPendingKill() const {
    return m_IsPendingKill;
}

World* Node::GetWorld() const {
    return m_World;
}

void Node::Destroy() {
    if (IsPendingKill()) {
        // The Node will already die at the end of the frame!
        return;
    }
    m_IsPendingKill = true;

    if (WasBeginPlayCalled()) {
        EndPlay();
    }

    Array<Node*> ChildrenToKill = m_Children;
    for (Node* child : ChildrenToKill) {
        AE_ASSERT(child);
        child->Destroy();
    }

    AE_ASSERT(GetWorld());
    AE_ASSERT(GetWorld()->m_PendingKills.Contains(this) == false);
    GetWorld()->m_PendingKills.Add(this);
}

Node* Node::CreateChild(const Class& InChildClass) {
    Node* node = Object::Create(InChildClass)->As<Node>();
    AE_ASSERT(node);
    AE_ASSERT(node->IsA<Node>(), "You can only add Nodes as Children.");

    if (Component* component = node->As<Component>()) {
        if (!GetClass().IsSubclassOf(component->GetRequiredParentClass())) {
            AE_ERROR("Cannot create Component, without given Parent Class '{0}'", component->GetRequiredParentClass().Name);
            delete node;
            return nullptr;
        }
    }

    node->SetParent(this, false);

    if (IsInitialized()) {
        node->InitializeNode(*GetWorld());
    }

    return node;
}

Node* Node::GetChild(int InIndex) const {
    if (InIndex < 0 || InIndex >= m_Children.Size())
        return nullptr;
    return m_Children[InIndex];
}

Node* Node::GetChildByName(const String& InName, bool InIncludeSelf) {
    if (GetName() == InName && InIncludeSelf)
        return this;

    for (int i = 0; i < GetChildCount(); i++) {
        Node* node = GetChild(i)->GetChildByName(InName, true);
        if (node) return node;
    }

    return nullptr;
}

Node* Node::GetChildByClass(const Class& InClass, bool InIncludeSelf) {
    if (this->IsA(InClass) && InIncludeSelf)
        return this;

    for (int i = 0; i < GetChildCount(); i++) {
        Node* node = GetChild(i)->GetChildByClass(InClass, true);
        if (node) return node;
    }

    return nullptr;
}

int32_t Node::GetSiblingIndex() const {
    if (!GetParent()) {
        return -1;
    }

    int32_t idx = 0;
    for (const Node* child : GetParent()->m_Children) {
        if (child == this) {
            return idx;
        }
        idx++;
    }

    AE_ASSERT(false, "Node is not a child of another Node, but should be!");
}

void Node::SetSiblingIndex(int32_t InIndex) {
    if (!GetParent()) {
        return;
    }
    Array<Node*>& siblings = GetParent()->m_Children;
    const int32_t current = siblings.IndexOf(this);
    if (current < 0) {
        return;
    }
    const int32_t last = siblings.Size() - 1;
    const int32_t clamped = InIndex < 0 ? 0 : (InIndex > last ? last : InIndex);
    if (clamped == current) {
        return;
    }
    siblings.RemoveAt(current);
    siblings.Insert(clamped, this);
}

uint32_t Node::GetChildCount() const {
    return m_Children.Size();
}

bool Node::HasChildren() const {
    return m_Children.Size() > 0;
}

Node* Node::GetParent() const {
    return m_Parent;
}

void Node::SetParent(Node* InParent, bool InKeepWorldTransform) {
    ForceSetParent(InParent, InKeepWorldTransform, true);
}

bool Node::IsChildOf(Node* InOther) const {
    return GetParent() ? (GetParent() == InOther ? true : GetParent()->IsChildOf(InOther)) : false;
}

Node* Node::GetRootNode() {
    return GetParent() ? GetParent()->GetRootNode() : this;
}

Node3D* Node::GetTransform() {
    return GetParent() ? GetParent()->GetTransform() : nullptr;
}

Node3D* Node::GetParentTransform() {
    return GetParent() ? GetParent()->GetTransform() : nullptr;
}

void Node::InitializeNode(World& OutWorld) {
    AE_ASSERT(GetWorld() == nullptr);
    /*if (GetName() == "New Node")
        SetName(GetClass().GetClassName());*/
    OutWorld.m_WorldNodes.Add(this);
    m_World = &OutWorld;
    m_Initialized = true;

    // Create issue to call BeginPlay
    OutWorld.m_BeginPlayIssues.Add(this);

    for (int32_t i = 0; i < GetChildCount(); i++) {
        GetChild(i)->InitializeNode(OutWorld);
    }
}

void Node::UnInitializeNode(World& OutWorld) {
    AE_ASSERT(GetWorld() == &OutWorld);
}

void Node::TickTransform(bool InInWorldSpace) {
    for (Node* child : m_Children) {
        if (child->m_Enabled) {
            child->m_EnabledInHierarchy = m_EnabledInHierarchy;
        }

        child->TickTransform(false);
    }
}

void Node::OnParentChange(Node* InPrev, Node* InNext) { }

void Node::ForceSetParent(Node* InParent, bool InKeepWorldTransform, bool InInGameplayContext) {
    if (InParent && InParent->IsChildOf(this)) {
        return;
    }
    if (InInGameplayContext) {
        OnParentChange(GetParent(), InParent);
    }

    // Capture the world pose *before* the hierarchy changes. World transforms are
    // derived from the parent chain, so once m_Parent moves the old world pose is
    // gone. Only meaningful when this node is itself a Node3D; for non-spatial
    // nodes GetTransform() walks up to a parent we must not touch.
    Node3D* transform = GetTransform();
    if (transform != this) {
        transform = nullptr;
    }
    const Mat4 worldBefore = (transform && InKeepWorldTransform) ? transform->GetTransformMatrix() : Mat4(1.0f);

    if (GetParent()) {
        GetParent()->m_Children.Remove(this);
    }

    m_Parent = InParent;

    if (GetParent()) {
        GetParent()->m_Children.Add(this);
    }

    // Reenable or Disable
    if (GetParent()) {
        if (m_Enabled && m_EnabledInHierarchy != GetParent()->m_EnabledInHierarchy) {
            m_EnabledInHierarchy = GetParent()->m_EnabledInHierarchy;
        }
    } else {
        m_EnabledInHierarchy = m_Enabled;
    }

    // Keep-world: re-express the captured world pose in the new parent's space.
    // Keep-local: nothing to do — local SRT is unchanged and world re-derives.
    if (transform && InKeepWorldTransform) {
        transform->SetTransformMatrix(worldBefore);
    }
}

bool Node::ShouldUpdateInCurrentContext() const {
    return IsEnabled() && WasBeginPlayCalled() && !IsPendingKill();
}