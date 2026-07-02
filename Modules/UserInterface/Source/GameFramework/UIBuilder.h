#pragma once
#include "UINode.h"
#include "UILabel.h"
#include "UIButton.h"
#include "UIForEach.h"
#include "UIIf.h"
#include "Common/Array.h"

/** Declarative, container-lambda helpers for building UI. Each helper creates a child on the given
 *  parent, configures it, and returns a reference so you can tweak it further. Data binding is via
 *  lambdas re-evaluated every frame (see UINode::Bind); For/If reconcile structure automatically.
 *
 *      UI::VStack(*root, [](UINode& v) {
 *          UI::Label(v, [] { return "Count: " + std::to_string(count); });
 *          UI::Button(v, "Add", [] { count++; });
 *          UI::If(v, [] { return count > 3; }, [](UINode& c) { UI::Label(c, [] { return String("Big!"); }); });
 *          UI::ForEach(v, [] { return count; }, [](UINode& item, int i) {
 *              UI::Label(item, [i] { return "Item " + std::to_string(i); });
 *          });
 *      });
 */
namespace UI {

/** A text label whose text is re-evaluated every frame. Fills its container by default (text is
 *  aligned inside via HAlign/VAlign); harmless for split children, which fill their slot anyway. */
inline UILabel& Label(UINode& InParent, std::function<String()> InText) {
    UILabel* label = InParent.Add<UILabel>();
    label->Fill();
    if (InText) {
        label->Bind = [label, text = std::move(InText)] { label->Text = text(); };
    }
    return *label;
}

/** A clickable button with a caption and an OnClick callback. */
inline UIButton& Button(UINode& InParent, const String& InCaption, std::function<void()> InOnClick) {
    UIButton* button = InParent.Add<UIButton>();
    button->SetCaption(InCaption);
    button->OnClick = std::move(InOnClick);
    return *button;
}

/** A container that fills its slot and stacks children vertically (top to bottom). */
inline UINode& VStack(UINode& InParent, const std::function<void(UINode&)>& InBody) {
    UINode* stack = InParent.Add<UINode>();
    stack->Fill();
    stack->LayoutMode = UILayoutMode::SplitY;
    if (InBody) {
        InBody(*stack);
    }
    return *stack;
}

/** A container that fills its slot and stacks children horizontally (left to right). */
inline UINode& HStack(UINode& InParent, const std::function<void(UINode&)>& InBody) {
    UINode* stack = InParent.Add<UINode>();
    stack->Fill();
    stack->LayoutMode = UILayoutMode::SplitX;
    if (InBody) {
        InBody(*stack);
    }
    return *stack;
}

/** A data-driven list: keeps one item per Count(), each built once by InBuildItem(item, index). */
inline UIForEach& ForEach(UINode& InParent, std::function<int()> InCount, std::function<void(UINode&, int)> InBuildItem) {
    UIForEach* list = InParent.Add<UIForEach>();
    list->Fill();
    list->Count = std::move(InCount);
    list->BuildItem = std::move(InBuildItem);
    return *list;
}

/** Sugar over ForEach bound to an Array<T> getter. Call with an explicit type: UI::ForEach<T>(...). */
template<typename T>
inline UIForEach& ForEach(UINode& InParent, std::function<const Array<T>&()> InItems, std::function<void(UINode&, const T&, int)> InBuildItem) {
    return ForEach(InParent,
        [InItems] { return InItems().Size(); },
        [InItems, build = std::move(InBuildItem)](UINode& item, int i) { build(item, InItems()[i], i); });
}

/** Conditional content: InBuild runs once; the subtree is shown only while InCond() is true. */
inline UIIf& If(UINode& InParent, std::function<bool()> InCond, std::function<void(UINode&)> InBuild) {
    UIIf* node = InParent.Add<UIIf>();
    node->Fill();
    node->Condition = std::move(InCond);
    node->Build = std::move(InBuild);
    return *node;
}

} // namespace UI
