#include "NodeRecord.h"
#include "GameFramework/Node.h"
#include "Serialization/Json.h"
#include "Core/Log.h"

using json = nlohmann::json;

static void CollectPropertyAssets(Property* InProperty, const json& InValue, Array<UUID>& OutIds) {
    if (!InProperty) {
        return;
    }

    if (WeakObjectPtrProperty* weak = Cast<WeakObjectPtrProperty>(InProperty)) {
        if (!InValue.is_string() || !weak->InnerClass.IsSubclassOf(Class("Asset"))) {
            return;
        }
        const UUID id = UUID::FromString(InValue.get<String>());
        if (id.IsValid() && !OutIds.Contains(id)) {
            OutIds.Add(id);
        }
        return;
    }

    if (ArrayProperty* array = Cast<ArrayProperty>(InProperty)) {
        if (InValue.is_array()) {
            for (const json& element : InValue) {
                CollectPropertyAssets(array->InnerProperty, element, OutIds);
            }
        }
        return;
    }

    if (StructProperty* structProperty = Cast<StructProperty>(InProperty)) {
        if (!InValue.is_object()) {
            return;
        }
        for (Property* inner : Property::GetTypeProperties(structProperty->InnerStructTypename)) {
            if (InValue.contains(inner->Name)) {
                CollectPropertyAssets(inner, InValue[inner->Name], OutIds);
            }
        }
    }
}

SharedObjectPtr<NodeRecord> NodeRecord::Capture(Node& InNode) {
    NodeRecord* record = new NodeRecord();
    record->Name = InNode.GetName();
    record->ClassName = InNode.GetSerializedClass().Name;
    record->Inherited = InNode.IsInherited();

    for (const String& name : InNode.GetOverriddenProperties()) {
        Property* property = Property::FindTypeProperty(InNode.GetClass(), name);
        if (!property) {
            continue;
        }
        record->Values[name] = JsonSerializer::SerializeProperty(property, property->GetValuePtr(&InNode));
    }

    for (uint32_t i = 0; i < InNode.GetChildCount(); i++) {
        SharedObjectPtr<NodeRecord> child = Capture(*InNode.GetChild((int)i));
        if (!child->IsEmptyPatch()) {
            record->Children.Add(child);
        }
    }

    return SharedObjectPtr<NodeRecord>(record);
}

Node* NodeRecord::Instantiate(Node* InParent) const {
    const Class nodeClass(ClassName);

    Node* node = InParent ? InParent->AttachChild(nodeClass) : Cast<Node>(Object::Create(nodeClass));
    if (!node) {
        AE_WARN("Scene data references unknown class '{0}' for node '{1}'", ClassName, Name);
        return nullptr;
    }

    if (!Name.empty()) {
        node->SetName(Name);
    }
    Apply(*node);
    return node;
}

void NodeRecord::Apply(Node& OutNode) const {
    for (const auto& [name, value] : Values.items()) {
        Property* property = Property::FindTypeProperty(OutNode.GetClass(), name);
        if (!property) {
            AE_WARN("Node '{0}' has no property '{1}' anymore; the stored override is dropped", Name, name);
            continue;
        }
        JsonSerializer::DeserializeProperty(property, property->GetValuePtr(&OutNode), value);
        property->NotifyChanged(&OutNode);
        OutNode.MarkPropertyOverridden(name);
    }

    for (const SharedObjectPtr<NodeRecord>& child : Children) {
        if (!child.Get()) {
            continue;
        }
        if (!child->Inherited) {
            child->Instantiate(&OutNode);
            continue;
        }

        Node* inherited = OutNode.GetDirectChildByName(child->Name);
        if (!inherited) {
            AE_WARN("Inherited node '{0}' no longer exists under '{1}'; its overrides are dropped", child->Name, Name);
            continue;
        }
        child->Apply(*inherited);
    }
}

bool NodeRecord::IsEmptyPatch() const {
    return Inherited && Values.empty() && Children.IsEmpty();
}

void NodeRecord::CollectAssetReferences(Array<UUID>& OutIds) const {
    const Class nodeClass(ClassName);
    if (nodeClass.IsBlueprint()) {
        const UUID id = nodeClass.GetBlueprintId();
        if (id.IsValid() && !OutIds.Contains(id)) {
            OutIds.Add(id);
        }
    }

    for (const auto& [name, value] : Values.items()) {
        CollectPropertyAssets(Property::FindTypeProperty(nodeClass, name), value, OutIds);
    }

    for (const SharedObjectPtr<NodeRecord>& child : Children) {
        if (child.Get()) {
            child->CollectAssetReferences(OutIds);
        }
    }
}

json NodeRecord::ToJson() const {
    json record = json::object();
    if (!Name.empty()) {
        record["Name"] = Name;
    }
    record["Class"] = ClassName;
    if (Inherited) {
        record["Inherited"] = true;
    }
    if (!Values.empty()) {
        record["Overrides"] = Values;
    }
    if (!Children.IsEmpty()) {
        json children = json::array();
        for (const SharedObjectPtr<NodeRecord>& child : Children) {
            if (child.Get()) {
                children.push_back(child->ToJson());
            }
        }
        record["Children"] = children;
    }
    return record;
}

SharedObjectPtr<NodeRecord> NodeRecord::FromJson(const json& InJson) {
    if (!InJson.is_object()) {
        return nullptr;
    }

    NodeRecord* record = new NodeRecord();
    record->Name = InJson.value("Name", String());
    record->ClassName = InJson.value("Class", String());
    record->Inherited = InJson.value("Inherited", false);
    if (InJson.contains("Overrides") && InJson["Overrides"].is_object()) {
        record->Values = InJson["Overrides"];
    }
    if (InJson.contains("Children") && InJson["Children"].is_array()) {
        for (const json& child : InJson["Children"]) {
            if (SharedObjectPtr<NodeRecord> childRecord = FromJson(child)) {
                record->Children.Add(childRecord);
            }
        }
    }
    return SharedObjectPtr<NodeRecord>(record);
}

std::vector<uint8_t> NodeRecord::ToBinary() const {
    return json::to_cbor(ToJson());
}

SharedObjectPtr<NodeRecord> NodeRecord::FromBinary(const std::vector<uint8_t>& InBytes) {
    const json parsed = json::from_cbor(InBytes, true, false);
    if (parsed.is_discarded()) {
        return nullptr;
    }
    return FromJson(parsed);
}
