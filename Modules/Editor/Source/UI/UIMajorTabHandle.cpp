#include "UIMajorTabHandle.h"
#include "EditorWindow.h"

void UIMajorTabHandle::OnPressed(const Vec2& InCursorPos) {
    m_PressPosition = InCursorPos;
}

void UIMajorTabHandle::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (!Tab || !OwnerWindow) {
        return;
    }
    if (OwnerWindow->IsDraggingTabHandle()) {
        OwnerWindow->UpdateTabHandleDrag(InCursorPos);
    } else if (glm::length(InCursorPos - m_PressPosition) > 8.0f) {
        OwnerWindow->BeginTabHandleDrag(Tab);
        OwnerWindow->UpdateTabHandleDrag(InCursorPos);
    }
}

void UIMajorTabHandle::OnReleased(bool InInside) {
    (void)InInside;
    if (Tab && OwnerWindow && OwnerWindow->IsDraggingTabHandle()) {
        OwnerWindow->EndTabHandleDrag(Tab);
    }
}
