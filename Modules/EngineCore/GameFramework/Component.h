#pragma once
#include "Node.h"
#include "Component.gen.h"

class World;

/** Component is a Node that adds behaviour or data to another Node rather than
 *  standing on its own in the scene.
 *
 *  A Component is always attached to a single host Node and stays with it for its
 *  whole lifetime: it cannot exist independently in a World and is never moved to
 *  a different host. A Component can restrict which kinds of Node it may attach
 *  to, allowing behaviour to be composed onto an entity from reusable pieces.
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