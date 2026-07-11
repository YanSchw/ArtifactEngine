#include "UICaptionButton.h"
#include "EditorStyle.h"
#include "Rendering/UIDrawList.h"

UICaptionButton::UICaptionButton() {
    Interactable = true;
}

void UICaptionButton::Paint(UIDrawList& OutDrawList) {
    const bool close = (Action == UICaptionAction::Close);
    if (IsHovered()) {
        OutDrawList.AddRect(m_Geometry, close ? EditorStyle::CaptionCloseHover : EditorStyle::CaptionHover, m_WorldMatrix);
    }

    const Vec4 color = (close && IsHovered()) ? EditorStyle::TextBright : EditorStyle::Text;
    const Vec2 center = m_Geometry.Center();
    const float half = 5.0f;

    switch (Action) {
        case UICaptionAction::Minimize: {
            OutDrawList.AddRect(UIRectF(center.x - half, center.y - 0.5f, half * 2.0f, 1.0f), color, m_WorldMatrix);
            break;
        }
        case UICaptionAction::Maximize: {
            const float x = center.x - half;
            const float y = center.y - half;
            const float size = half * 2.0f;
            OutDrawList.AddRect(UIRectF(x, y, size, 1.0f), color, m_WorldMatrix);
            OutDrawList.AddRect(UIRectF(x, y + size - 1.0f, size, 1.0f), color, m_WorldMatrix);
            OutDrawList.AddRect(UIRectF(x, y + 1.0f, 1.0f, size - 2.0f), color, m_WorldMatrix);
            OutDrawList.AddRect(UIRectF(x + size - 1.0f, y + 1.0f, 1.0f, size - 2.0f), color, m_WorldMatrix);
            break;
        }
        case UICaptionAction::Close: {
            const float length = 13.0f;
            for (float angle : { 45.0f, -45.0f }) {
                const Mat4 transform = m_WorldMatrix
                    * glm::translate(Mat4(1.0f), Vec3(center, 0.0f))
                    * glm::rotate(Mat4(1.0f), glm::radians(angle), Vec3(0.0f, 0.0f, 1.0f))
                    * glm::translate(Mat4(1.0f), Vec3(-center, 0.0f));
                OutDrawList.AddRect(UIRectF(center.x - length * 0.5f, center.y - 0.5f, length, 1.0f), color, transform);
            }
            break;
        }
    }
}
