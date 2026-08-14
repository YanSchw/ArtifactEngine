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
        s_NodeIcons["BoxShapeNode"] = BoxShape();
        s_NodeIcons["SphereShapeNode"] = SphereShape();
        s_NodeIcons["CapsuleShapeNode"] = CapsuleShape();
        s_NodeIcons["CylinderShapeNode"] = CylinderShape();
        s_NodeIcons["RigidBodyNode"] = RigidBody();
        s_NodeIcons["CharacterNode"] = Character();
        s_NodeIcons["UINode"] = Get("d5ff8074-f233-41a8-b202-f6ced99f2d8f");
        s_NodeIcons["UICanvas"] = Get("26818e74-f484-4f52-8f97-b06dcb64c58b");
        s_NodeIcons["UIQuad"] = Get("62670d8d-e97b-4c2e-afdd-3f69ebbddb83");
        s_NodeIcons["UILabel"] = Get("177e97c2-406e-4cd6-8642-725b023d56db");
        s_NodeIcons["UIButton"] = Get("310550f5-b61a-4785-bebf-7fafbd2bcfdc");
        s_NodeIcons["UIImage"] = Get("ffd11530-3e99-465a-9bc7-977bd99eda64");
        s_NodeIcons["UISvg"] = Get("ab4214ae-5b25-4488-a8fb-2795ac6ccc1d");
        s_NodeIcons["UIStack"] = Get("db37f8bf-0d81-48b2-83eb-ed2008128b39");
        s_NodeIcons["UIHStack"] = Get("b0499709-b4a7-4f1c-8657-98d93b312e36");
        s_NodeIcons["UIVStack"] = Get("e76ce95f-e3fb-4d95-9bc8-7d890d4295a5");
        s_NodeIcons["UIScrollArea"] = Get("f086650c-44a5-4861-8051-fc62d6a820c2");
        s_NodeIcons["UITextArea"] = Get("536de86e-7d42-4604-8d9d-e7b2d25ab405");
        s_NodeIcons["UIToggle"] = Get("c2d48cb1-c5bf-4b93-bb8d-2b7f41919506");
        s_NodeIcons["UIForEach"] = Get("ed702f8c-5e30-4ee3-9ec9-da846f54fef5");
        s_NodeIcons["UIIf"] = Get("34ebcbd5-8be0-457c-88c3-874d6ece94a4");
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
        s_AssetIcons["Material"] = Material();
        s_AssetIcons["ShaderGraph"] = GraphEditor();
        s_AssetIcons["Animation"] = Animation();
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
        s_AssetColors["Material"] = HexColor(0xC98BE0);
        s_AssetColors["ShaderGraph"] = HexColor(0xD86BA9);
        s_AssetColors["Animation"] = HexColor(0x6FD84A);
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
