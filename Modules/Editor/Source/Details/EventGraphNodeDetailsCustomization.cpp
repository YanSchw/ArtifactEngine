#include "EventGraphNodeDetailsCustomization.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "Tabs/MajorTab.h"
#include "Assets/NodeAsset.h"
#include "Scripting/EventGraph.h"
#include "Scripting/EventGraphNode.h"
#include "GameFramework/UILabel.h"
#include "UI/EditorStyle.h"

Class EventGraphNodeDetailsCustomization::GetSupportedClass() const {
    return EventGraphNode::StaticClass();
}

void EventGraphNodeDetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    EventGraphNode* node = Cast<EventGraphNode>(InObject);
    MajorTab* major = InTab.GetMajorTab();
    NodeAsset* asset = major ? Cast<NodeAsset>(major->GetEditedAsset()) : nullptr;
    EventGraph* graph = asset ? asset->GetEventGraph() : nullptr;
    if (!node) {
        return;
    }

    DetailsCategory& category = AddCategory(InList, InTab, "Inputs", 0);
    const std::shared_ptr<void> frame = node->MakeFrame();
    const WeakObjectPtr<Object> weakNode = node;
    const WeakObjectPtr<MajorTab> weakMajor = major;
    const WeakObjectPtr<EventGraph> weakGraph = graph;
    int32_t rows = 0;
    for (const SharedObjectPtr<GraphPin>& pin : node->Pins) {
        if (!node->HasEditableDefault(*pin) || (graph && graph->IsPinConnected(*node, *pin))) {
            continue;
        }
        Property* property = node->GetPinValue(frame.get(), pin->Name).ValueProperty;
        const String pinName = pin->Name;
        const String label = node->GetPinLabel(*pin);
        DetailsRow& row = AddRow(*category.GetBody(), InTab, label, 0);

        DetailsEditHandler handler;
        handler.BeginEdit = [weakMajor, weakGraph, label] {
            if (weakMajor.Get() && weakGraph.Get()) {
                weakMajor.Get()->BeginTransaction("Edit " + label, weakGraph.Get());
            }
        };
        handler.CommitEdit = [weakNode, pinName] {
            if (EventGraphNode* edited = Cast<EventGraphNode>(weakNode.Get())) {
                edited->CommitDefaultValue(pinName);
            }
        };
        const uint64_t offset = MakeExternalOffset(std::to_string((uintptr_t)node) + ":" + pinName, [pinName](Object* InNode) -> void* {
            EventGraphNode* target = Cast<EventGraphNode>(InNode);
            return target ? target->GetDefaultValuePtr(pinName) : nullptr;
        });
        BuildValueEditor(*row.GetValueHost(), weakNode, offset, property, handler, label);
        rows++;
    }

    if (rows == 0) {
        UILabel* empty = AddRow(*category.GetBody(), InTab, "No unconnected inputs", 0).GetValueHost()->Add<UILabel>();
        empty->Fill();
        empty->Color = EditorStyle::TextDim;
    }
}
