#include "UIThumbnail.h"
#include "Rendering/Texture.h"

void UIThumbnail::OnUIUpdate(const UIFrameContext& InContext) {
    UIImage::OnUIUpdate(InContext);

    Texture* texture = nullptr;
    if (Resolve && IsOnScreen()) {
        texture = Resolve(m_Held);
    } else {
        m_Held = AssetStreamHandle();
    }
    const bool ready = texture && texture->GetDefaultView().Get();
    Image = ready ? texture : nullptr;
    if (Fallback) {
        Fallback->SetEnabled(!ready);
    }
}

void UIThumbnail::Paint(UIDrawList& OutDrawList) {
    if (Image.Get()) {
        UIImage::Paint(OutDrawList);
    }
}

bool UIThumbnail::IsOnScreen() {
    const Vec2 min = m_Geometry.Min();
    const Vec2 max = m_Geometry.Max();
    if (max.x <= min.x || max.y <= min.y) {
        return false;
    }
    for (Node* node = GetParent(); node; node = node->GetParent()) {
        UINode* ancestor = node->As<UINode>();
        if (!ancestor || !ancestor->ClipChildren) {
            continue;
        }
        const UIRectF clip = ancestor->GetContentRect();
        if (max.x <= clip.Min().x || min.x >= clip.Max().x || max.y <= clip.Min().y || min.y >= clip.Max().y) {
            return false;
        }
    }
    return true;
}
