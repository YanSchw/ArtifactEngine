#include "UIForEach.h"

void UIForEach::OnBind() {
    UINode::OnBind();

    const int want = Count ? std::max(0, Count()) : 0;
    int have = (int)GetChildCount();

    // Grow: build new item roots once (their live values come from Bind lambdas inside BuildItem).
    while (have < want) {
        UINode* item = Add<UINode>()->Fill();
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
