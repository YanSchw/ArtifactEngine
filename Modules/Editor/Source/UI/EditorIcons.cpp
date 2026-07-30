#include "EditorIcons.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

VectorImage* EditorIcons::GetNodeIcon(const Class& InClass) {
    static Map<String, VectorImage*> s_NodeIcons;
    AssetManager& assets = AssetManager::Get();
    if (s_NodeIcons.Size() == 0) {
        s_NodeIcons["Node"] = Get("b1c2d3e4-0001-4a00-9000-000000000001");
        s_NodeIcons["Node3D"] = Get("b1c2d3e4-0009-4a00-9000-000000000014");
        s_NodeIcons["StaticMeshNode"] = Get("b1c2d3e4-000f-4a00-9000-00000000000f");
        s_NodeIcons["CameraNode"] = Get("f1c0210c-a27e-4d46-8b20-9fdf39a88193");
        s_NodeIcons["Component"] = Get("b1c2d3e4-0009-4a00-9000-000000000012");
    }

    return s_NodeIcons.ContainsKey(InClass.Name) ? s_NodeIcons[InClass.Name] : GetNodeIcon(InClass.GetParentClass());
}

void EditorIcons::Paint(UIDrawList& OutDrawList, VectorImage* InIcon, const UIRectF& InRect,
                        const Vec4& InTint, const Mat4& InTransform) {
    if (!InIcon || !InIcon->IsLoaded() || InTint.a <= 0.0f) {
        return;
    }
    const Vec2 documentSize = InIcon->GetSize();
    if (documentSize.x <= 0.0f || documentSize.y <= 0.0f || InRect.Size.x <= 0.0f || InRect.Size.y <= 0.0f) {
        return;
    }

    const Vec2 fit = InRect.Size / documentSize;
    const Vec2 scale = Vec2(std::min(fit.x, fit.y));
    const Vec2 topLeft = InRect.Position + (InRect.Size - documentSize * scale) * 0.5f;

    struct CachedMesh {
        SvgMesh Mesh;
        float Scale = 0.0f;
    };
    static Map<VectorImage*, CachedMesh> s_Cache;

    CachedMesh& cached = s_Cache[InIcon];
    const float detailScale = std::max(scale.x, scale.y);
    if (cached.Scale <= 0.0f || detailScale > cached.Scale * 1.25f || detailScale < cached.Scale * 0.8f) {
        InIcon->Tessellate(detailScale, cached.Mesh);
        cached.Scale = detailScale;
    }
    if (cached.Mesh.Indices.IsEmpty()) {
        return;
    }

    OutDrawList.AddTriangles(&cached.Mesh.Positions[0], &cached.Mesh.Colors[0], cached.Mesh.Positions.Size(),
                             &cached.Mesh.Indices[0], cached.Mesh.Indices.Size(), InTint, topLeft, scale, InTransform);
}
