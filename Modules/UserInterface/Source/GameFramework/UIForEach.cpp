#include "UIForEach.h"

UIForEach::UIForEach() {
    LayoutMode = UILayoutMode::SplitY; // vertical list by default; set SplitX for a row
}

void UIForEach::OnBind() {
    UINode::OnBind();

    const int want = Count ? std::max(0, Count()) : 0;
    int have = (int)GetChildCount();

    // Grow: build new item roots once (their live values come from Bind lambdas inside BuildItem).
    while (have < want) {
        UINode* item = Add<UINode>();
        if (BuildItem) {
            BuildItem(*item, have);
        }
        have++;
    }
    // Shrink: delete the trailing items (Node's destructor unparents + frees their subtrees).
    while (have > want) {
        delete GetChild(have - 1);
        have--;
    }
}
