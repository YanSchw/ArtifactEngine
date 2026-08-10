#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "Common/String.h"
#include "EditorTransaction.gen.h"

class MajorTab;
class NodeRecord;

/** One recorded object: its property values, plus its children when it is a Node. */
struct EditorSnapshot {
    WeakObjectPtr<Object> Target;
    String Values;
    SharedObjectPtr<NodeRecord> Children;
};

/** One undo step of a MajorTab; never built directly use MajorTab::BeginTransaction instead */
class EditorTransaction : public Object {
public:
    ARTIFACT_CLASS();

    explicit EditorTransaction(const String& InTitle) : Title(InTitle) {}

    String Title;

    void Record(Object* InObject);
    bool IsUnchanged() const;
    void Restore(MajorTab& InTab);

private:
    static EditorSnapshot Capture(Object* InObject);
    static void Apply(const EditorSnapshot& InSnapshot);

    Array<EditorSnapshot> m_Snapshots;
};
