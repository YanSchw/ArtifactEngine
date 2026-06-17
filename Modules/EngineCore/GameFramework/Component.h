#pragma once
#include "Node.h"
#include "Component.gen.h"

class World;

/** Component is a special kind of Node.
 *  Normal Nodes own their children and control flows from parent to child.
 *  A Component may never have children and will always have a parent;
 *  it may never be just spawned in the World.
 */
class Component : public Node {
public:
    ARTIFACT_CLASS();

    virtual void SetParent(Node* InParent, bool InKeepWorldTransform = true) override;

    void SetRequiredParentClass(const Class& InParentClass);
    Class GetRequiredParentClass() const;
private:
    Class m_ParentBaseclass = Node::StaticClass();
};