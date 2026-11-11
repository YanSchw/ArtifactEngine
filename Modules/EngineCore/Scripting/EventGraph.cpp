#include "EventGraph.h"
#include "EventGraphNodes.h"
#include "GameFramework/Node.h"
#include <cctype>

static const char* s_BuiltinPinStructs[] = { "Vec2", "Vec3", "Vec4", "Color", "Quat" };

EventGraph::EventGraph() {
    GraphName = "Event Graph";
}

String EventGraph::PinTypeOf(Property* InProperty) {
    if (Cast<BoolProperty>(InProperty)) {
        return "Bool";
    }
    if (IntProperty* integer = Cast<IntProperty>(InProperty)) {
        return !integer->IsUnsigned && integer->NumBits == 32 ? String("Int")
             : (integer->IsUnsigned ? "UInt" : "Int") + std::to_string(integer->NumBits);
    }
    if (FloatProperty* number = Cast<FloatProperty>(InProperty)) {
        return number->IsDouble ? "Double" : "Float";
    }
    if (Cast<StringProperty>(InProperty)) {
        return "String";
    }
    if (Cast<UUIDProperty>(InProperty)) {
        return "UUID";
    }
    if (EnumProperty* enumeration = Cast<EnumProperty>(InProperty)) {
        return "Enum:" + enumeration->InnerEnumTypename;
    }
    if (StructProperty* structure = Cast<StructProperty>(InProperty)) {
        for (const char* builtin : s_BuiltinPinStructs) {
            if (structure->InnerStructTypename == builtin) {
                return builtin;
            }
        }
        return "Struct:" + structure->InnerStructTypename;
    }
    if (WeakObjectPtrProperty* weak = Cast<WeakObjectPtrProperty>(InProperty)) {
        return ObjectPinType(weak->InnerClass);
    }
    if (SharedObjectPtrProperty* shared = Cast<SharedObjectPtrProperty>(InProperty)) {
        return ObjectPinType(shared->InnerClass);
    }
    if (SubclassOfProperty* subclass = Cast<SubclassOfProperty>(InProperty)) {
        return "Class:" + subclass->BaseClass.Name;
    }
    if (ArrayProperty* array = Cast<ArrayProperty>(InProperty)) {
        return "Array:" + PinTypeOf(array->InnerProperty);
    }
    if (MapProperty* map = Cast<MapProperty>(InProperty)) {
        return "Map:" + PinTypeOf(map->KeyProperty) + "," + PinTypeOf(map->ValueProperty);
    }
    return InProperty ? InProperty->GetClass().Name : String();
}

String EventGraph::PrettyName(const String& InName) {
    String name = InName.starts_with("m_") ? InName.substr(2) : InName;
    for (const char* prefix : { "In", "Out" }) {
        const size_t length = std::char_traits<char>::length(prefix);
        if (name.size() > length && name.starts_with(prefix) && std::isupper((unsigned char)name[length])) {
            name = name.substr(length);
        }
    }
    String pretty;
    for (size_t i = 0; i < name.size(); i++) {
        if (i > 0 && std::isupper((unsigned char)name[i]) && std::islower((unsigned char)name[i - 1])) {
            pretty += ' ';
        }
        pretty += name[i];
    }
    return pretty;
}

void EventGraph::RefreshNodes() {
    for (const SharedObjectPtr<GraphNode>& graphNode : Nodes) {
        EventGraphNode* node = Cast<EventGraphNode>(graphNode.Get());
        if (!node || !node->IsResolved()) {
            continue;
        }
        if (SelfNode* self = Cast<SelfNode>(node)) {
            self->SelfClass = SelfClass;
        }
        node->Pins.Clear();
        node->ConstructPins();
    }
    PruneInvalidConnections();
}

static bool IsSubtype(const String& InType, const String& InBaseType, const char* InPrefix) {
    const size_t length = std::char_traits<char>::length(InPrefix);
    return InType.starts_with(InPrefix) && InBaseType.starts_with(InPrefix)
        && Class(InType.substr(length)).IsSubclassOf(Class(InBaseType.substr(length)));
}

bool EventGraph::ArePinTypesCompatible(const String& InOutputType, const String& InInputType) const {
    if (InOutputType == InInputType) {
        return true;
    }
    if ((InOutputType == "Color" && InInputType == "Vec4") || (InOutputType == "Vec4" && InInputType == "Color")) {
        return true;
    }
    return IsSubtype(InOutputType, InInputType, "Object:") || IsSubtype(InOutputType, InInputType, "Class:");
}

bool EventGraph::AllowsNodeClass(const Class& InNodeClass) const {
    return InNodeClass.IsSubclassOf(EventGraphNode::StaticClass());
}

bool EventGraph::AllowsMultipleConnections(const GraphPin& InPin) const {
    return InPin.TypeName == ExecType ? InPin.IsInput() : InPin.IsOutput();
}

static void CollectSceneNodes(Node& InNode, Array<Node*>& OutNodes) {
    OutNodes.Add(&InNode);
    for (uint32_t i = 0; i < InNode.GetChildCount(); i++) {
        CollectSceneNodes(*InNode.GetChild((int)i), OutNodes);
    }
}

static bool IsEditorClass(const Class& InClass) {
    return InClass.GetModuleName() == "Editor";
}

void EventGraph::CollectNodeTemplates(Array<GraphNodeTemplate>& OutTemplates, Object* InContext) const {
    const auto add = [&OutTemplates](const String& InCategory, const String& InTitle, const Class& InClass,
                                     std::function<void(GraphNode&)> InConfigure, const String& InTooltip = String()) {
        OutTemplates.Add({ InCategory, InTitle, InTooltip, InClass, std::move(InConfigure) });
    };

    add("Events", "Event BeginPlay", EventBeginPlayNode::StaticClass(), nullptr);
    add("Events", "Event EndPlay", EventEndPlayNode::StaticClass(), nullptr);
    add("Events", "Event Tick", EventTickNode::StaticClass(), nullptr);
    add("Flow Control", "Branch", BranchNode::StaticClass(), nullptr);

    const String selfClass = SelfClass;
    add("Scene", "Self", SelfNode::StaticClass(), [selfClass](GraphNode& OutNode) { Cast<SelfNode>(&OutNode)->SelfClass = selfClass; });

    Array<Node*> sceneNodes;
    if (Node* root = Cast<Node>(InContext)) {
        CollectSceneNodes(*root, sceneNodes);
    }
    for (Node* sceneNode : sceneNodes) {
        const bool isSelf = sceneNode == InContext;
        const String name = isSelf ? String() : sceneNode->GetName();
        const String nodeClass = sceneNode->GetClass().Name;
        if (!isSelf) {
            add("Scene", "Get " + name, NodeReferenceNode::StaticClass(), [name, nodeClass](GraphNode& OutNode) {
                NodeReferenceNode* reference = Cast<NodeReferenceNode>(&OutNode);
                reference->NodeName = name;
                reference->NodeClass = nodeClass;
            });
        }
        for (Property* property : Property::GetAllTypeProperties(sceneNode->GetClass())) {
            if (!Cast<DelegateProperty>(property)) {
                continue;
            }
            const String delegateName = property->Name;
            const String title = PrettyName(delegateName) + (isSelf ? String() : " (" + name + ")");
            add(isSelf ? String("Events") : "Events|" + name, title, DelegateEventNode::StaticClass(), [name, delegateName, nodeClass](GraphNode& OutNode) {
                DelegateEventNode* event = Cast<DelegateEventNode>(&OutNode);
                event->NodeName = name;
                event->DelegateName = delegateName;
                event->OwnerClass = nodeClass;
            }, property->Comment);
        }
    }

    for (Function* function : Function::GetAllFunctions()) {
        const Class owner(function->OwnerClass);
        if (IsEditorClass(owner)) {
            continue;
        }
        const String category = function->Category.empty() ? PrettyName(owner.GetDisplayName()) : function->Category;
        const String functionClass = function->OwnerClass;
        const String functionName = function->Name;
        add(category, PrettyName(function->Name), CallFunctionNode::StaticClass(), [functionClass, functionName](GraphNode& OutNode) {
            CallFunctionNode* call = Cast<CallFunctionNode>(&OutNode);
            call->FunctionClass = functionClass;
            call->FunctionName = functionName;
        }, function->Comment);
    }

    for (const Class& nodeClass : Class::GetSubclassesOf(Node::StaticClass())) {
        if (IsEditorClass(nodeClass)) {
            continue;
        }
        const String className = nodeClass.Name;
        add("Casting", "Cast To " + className, CastNode::StaticClass(), [className](GraphNode& OutNode) {
            Cast<CastNode>(&OutNode)->TargetClass = className;
        });
    }
}
