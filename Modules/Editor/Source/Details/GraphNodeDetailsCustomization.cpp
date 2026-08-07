#include "GraphNodeDetailsCustomization.h"

#include "UI/EditorStyle.h"
#include "GameFramework/UILabel.h"

float GraphNodeDetailsCustomization::BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) {
    (void)InTab;

    UILabel* label = InHeader.Add<UILabel>();
    label->Fill();
    label->Position = Vec2(10.0f, 0.0f);
    label->FontSize = EditorStyle::FontSize + 1.0f;
    label->Color = EditorStyle::TextBright;
    label->VAlign = UIVAlign::Middle;
    label->Text = Cast<GraphNode>(InObject)->GetTitle();
    return 32.0f;
}

bool GraphNodeDetailsCustomization::WantsClassCategory(const Class& InClass) const {
    return InClass != GraphNode::StaticClass() && DetailsCustomization::WantsClassCategory(InClass);
}
