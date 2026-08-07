#include "ShaderGraphValueDetailsCustomization.h"

#include "DetailsCategory.h"

void ShaderGraphValueDetailsCustomization::BuildClassCategory(DetailsCategory& InCategory, const Class& InClass,
                                                              Object* InObject, DetailsTab& InTab) {
    ShaderGraphValueNode* node = Cast<ShaderGraphValueNode>(InObject);
    if (!node || InClass != ShaderGraphValueNode::StaticClass()) {
        GraphNodeDetailsCustomization::BuildClassCategory(InCategory, InClass, InObject, InTab);
        return;
    }

    const WeakObjectPtr<Object> weak(InObject);
    UINode& body = *InCategory.GetBody();

    auto addRow = [&](const char* InName) {
        if (Property* property = Property::FindTypeProperty(InClass, InName)) {
            AddPropertyRow(body, InTab, weak, 0, property, 0);
        }
    };

    addRow("ValueType");

    if (node->IsTexture()) {
        addRow("Texture");
    } else if (Property* value = Property::FindTypeProperty(InClass, "Value")) {
        static const char* s_Axes[] = { "X", "Y", "Z", "W" };
        const Array<Property*> axes = Property::GetTypeProperties("Vec4");
        const uint32_t count = ShaderValue::GetComponentCount(node->ValueType);

        for (uint32_t i = 0; i < count && (int32_t)i < axes.Size(); i++) {
            AddPropertyRow(body, InTab, weak, value->Offset, axes[i], 0, value,
                           count == 1 ? "Value" : s_Axes[i]);
        }
    }

    addRow("ExposeAsInput");
    if (node->ExposeAsInput) {
        addRow("InputName");
    }
}
