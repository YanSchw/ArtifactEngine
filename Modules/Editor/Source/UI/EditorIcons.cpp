#include "EditorIcons.h"
#include "EditorStyle.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

template<typename T>
static bool FindForClassChain(Map<String, T>& InTable, const Class& InClass, T& OutValue) {
    for (Class current = InClass; current != Class::None; current = current.GetParentClass()) {
        if (InTable.ContainsKey(current.Name)) {
            OutValue = InTable[current.Name];
            return true;
        }
    }
    return false;
}

VectorImage* EditorIcons::GetNodeIcon(const Class& InClass) {
    static Map<String, VectorImage*> s_NodeIcons;
    if (s_NodeIcons.Size() == 0) {
        s_NodeIcons["Node"] = Get("b1c2d3e4-0001-4a00-9000-000000000001");
        s_NodeIcons["Node3D"] = Get("b1c2d3e4-0009-4a00-9000-000000000014");
        s_NodeIcons["StaticMeshNode"] = Get("b1c2d3e4-000f-4a00-9000-00000000000f");
        s_NodeIcons["CameraNode"] = Get("f1c0210c-a27e-4d46-8b20-9fdf39a88193");
        s_NodeIcons["Component"] = Get("b1c2d3e4-0009-4a00-9000-000000000012");
    }

    VectorImage* icon = nullptr;
    return FindForClassChain(s_NodeIcons, InClass, icon) ? icon : Node();
}

VectorImage* EditorIcons::GetAssetIcon(const Class& InClass) {
    static Map<String, VectorImage*> s_AssetIcons;
    if (s_AssetIcons.Size() == 0) {
        s_AssetIcons["Asset"] = Asset();
        s_AssetIcons["Mesh"] = Mesh();
        s_AssetIcons["Texture2D"] = Texture();
        s_AssetIcons["Font"] = Font();
        s_AssetIcons["VectorImage"] = Node();
        s_AssetIcons["Scene"] = Level();
        s_AssetIcons["Blueprint"] = Node();
        s_AssetIcons["ShaderGraph"] = GraphEditor();
    }

    VectorImage* icon = nullptr;
    return FindForClassChain(s_AssetIcons, InClass, icon) ? icon : Asset();
}

Vec4 EditorIcons::GetAssetColor(const Class& InClass) {
    static Map<String, Vec4> s_AssetColors;
    if (s_AssetColors.Size() == 0) {
        s_AssetColors["Asset"] = HexColor(0x9A9A9A);
        s_AssetColors["Mesh"] = HexColor(0x1FB8C4);
        s_AssetColors["Texture2D"] = HexColor(0xE0704A);
        s_AssetColors["Font"] = HexColor(0xA96BD8);
        s_AssetColors["VectorImage"] = HexColor(0x4ACF8B);
        s_AssetColors["Scene"] = HexColor(0xE0A44A);
        s_AssetColors["Blueprint"] = HexColor(0x3D8BE0);
        s_AssetColors["ShaderGraph"] = HexColor(0xD86BA9);
    }

    Vec4 color(0.0f);
    return FindForClassChain(s_AssetColors, InClass, color) ? color : HexColor(0x9A9A9A);
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
