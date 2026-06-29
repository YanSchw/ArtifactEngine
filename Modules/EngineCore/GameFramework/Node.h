#pragma once
#include "Object/Object.h"
#include "Object/Enum.h"
#include "Node.gen.h"

class World;
class Node3D;
class Component;

ARTIFACT_ENUM();
enum class UpdateFlag : uint8_t {
    NeverUpdate = 0,
    WorldUpdate = 1,
    LocalUpdate = 2
};

/** Baseclass for all Nodes in the GameFramework */
class Node : public Object {
public:
    ARTIFACT_CLASS();

    Node();
    virtual ~Node();

    String GetName() const;
    void SetName(const String& InName);

    /** Gameplay Events */

    /** Called the very first frame the Node is active in the world
     *  GetWorld() will return a valid value and gameplay stuff may happen */
    virtual void BeginPlay() { }

    /** Called before the Node is removed from the World
     *  Last chance to do gameplay stuff */
    virtual void EndPlay() { }

    /** WorldUpdate is called every frame on the main thread
     *  UpdateFlag::WorldUpdate needs to be set first */
    virtual void WorldUpdate(float InDeltatime);

    void SetUpdateFlag(UpdateFlag InFlag);
    bool IsUpdateFlagSet(UpdateFlag InFlag) const;

    bool IsInitialized() const;
    bool IsEnabled() const;
    void SetEnabled(bool InEnabled);
    bool WasBeginPlayCalled() const;
    bool IsPendingKill() const;

    World* GetWorld() const;
    void Destroy();

    Node* CreateChild(const Class& InChildClass);

    Node* GetChild(int InIndex) const;
    Node* GetChildByName(const String& InName, bool InIncludeSelf = false);
    Node* GetChildByClass(const Class& InClass, bool InIncludeSelf = false);
    int32_t GetSiblingIndex() const;
    uint32_t GetChildCount() const;
    bool HasChildren() const;

    Node* GetParent() const;
    virtual void SetParent(Node* InParent, bool InKeepWorldTransform = true);
    bool IsChildOf(Node* InOther) const;
    Node* GetRootNode();

    virtual Node3D* GetTransform();
    virtual Node3D* GetParentTransform();

protected:
    virtual void InitializeNode(World& OutWorld);
    virtual void UnInitializeNode(World& OutWorld);

    virtual void TickTransform(bool InInWorldSpace = true);
    virtual void OnParentChange(Node* InPrev, Node* InNext);

    /** Always sets the Parent of the node, even for Components
     *  If InGameplayContext is set to 'true', Events will fire and Gameplay code might execute... */
    void ForceSetParent(Node* InParent, bool InKeepWorldTransform, bool InInGameplayContext);

private:
    /** Returns 'true', if the World is allowed to Update this Node in current Context
     *  Context: IsEnabled? WasBeginPlayCalled? IsNotPendingKill? etc. */
    bool ShouldUpdateInCurrentContext() const;

private:
    String m_Name = "";
    Node* m_Parent = nullptr;
    Array<Node*> m_Children;

    World* m_World = nullptr;
    UpdateFlag m_UpdateFlags = UpdateFlag::NeverUpdate;
    bool m_Initialized = false;
    bool m_Enabled = true, m_EnabledInHierarchy = true;
    bool m_WasBeginPlayCalled = false;
    bool m_IsPendingKill = false;

    friend class World;
};