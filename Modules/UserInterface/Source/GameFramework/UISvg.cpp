#include "UISvg.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

void UISvg::Paint(UIDrawList& OutDrawList) {
    if (!Image || !Image->IsLoaded() || Tint.a <= 0.0f) {
        return;
    }
    const Vec2 documentSize = Image->GetSize();
    const UIRectF& rect = m_Geometry;
    if (documentSize.x <= 0.0f || documentSize.y <= 0.0f || rect.Size.x <= 0.0f || rect.Size.y <= 0.0f) {
        return;
    }

    Vec2 scale = rect.Size / documentSize;
    Vec2 topLeft = rect.Position;
    if (PreserveAspect) {
        scale = Vec2(std::min(scale.x, scale.y));
        topLeft += (rect.Size - documentSize * scale) * 0.5f;
    }

    const float detailScale = std::max(scale.x, scale.y);
    const bool scaleChanged = m_CachedScale <= 0.0f ||
        detailScale > m_CachedScale * 1.25f || detailScale < m_CachedScale * 0.8f;
    if (Image.Get() != m_CachedImage || scaleChanged) {
        Image->Tessellate(detailScale, m_Mesh);
        m_CachedImage = Image.Get();
        m_CachedScale = detailScale;
    }
    if (m_Mesh.Indices.IsEmpty()) {
        return;
    }

    OutDrawList.AddTriangles(&m_Mesh.Positions[0], &m_Mesh.Colors[0], m_Mesh.Positions.Size(),
                             &m_Mesh.Indices[0], m_Mesh.Indices.Size(), Tint, topLeft, scale, m_WorldMatrix);
}
