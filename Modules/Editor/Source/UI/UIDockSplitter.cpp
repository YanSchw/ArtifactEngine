#include "UIDockSplitter.h"
#include "UIDockNode.h"
#include "EditorStyle.h"
#include "Rendering/UIDrawList.h"

UIDockSplitter::UIDockSplitter() {
    Fill();
    Interactable = true;
}

void UIDockSplitter::Paint(UIDrawList& OutDrawList) {
    const Vec4 color = (IsPressed() || IsHovered()) ? EditorStyle::Accent : EditorStyle::Border;
    OutDrawList.AddRect(m_Geometry, color, m_WorldMatrix);
}

void UIDockSplitter::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InCursorPos;
    if (UIDockNode* split = GetParent()->As<UIDockNode>()) {
        split->AdjustSplit(InDelta);
    }
}
