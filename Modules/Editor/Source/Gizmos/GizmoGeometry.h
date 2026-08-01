#pragma once
#include "Common/Array.h"
#include "Common/Types.h"
#include "Rendering/ShaderDataType.h"

struct GizmoVertex {
    Vec3 Position;
    Vec4 Color;

    static Array<ShaderDataType> GetLayout() {
        return { ShaderDataType::Float3, ShaderDataType::Float4 };
    }
};

class GizmoGeometry {
public:
    void Clear();
    bool IsEmpty() const { return m_Indices.IsEmpty(); }
    const Array<GizmoVertex>& GetVertices() const { return m_Vertices; }
    const Array<uint32_t>& GetIndices() const { return m_Indices; }

    void SetViewer(const Vec3& InEyePosition, const Vec3& InViewDirection, bool InOrthographic);

    void AddTriangle(const Vec3& InA, const Vec3& InB, const Vec3& InC, const Vec4& InColor);
    void AddQuad(const Vec3& InA, const Vec3& InB, const Vec3& InC, const Vec3& InD, const Vec4& InColor);
    void AddRibbon(const Array<Vec3>& InPoints, bool InClosed, float InHalfWidth, const Vec4& InColor);
    void AddLine(const Vec3& InFrom, const Vec3& InTo, float InHalfWidth, const Vec4& InColor);
    void AddDisc(const Vec3& InCenter, float InRadius, const Vec4& InColor);

    void AddCone(const Vec3& InBase, const Vec3& InTip, float InRadius, const Vec4& InColor);
    void AddBox(const Vec3& InCenter, const Vec3& InHalfX, const Vec3& InHalfY, const Vec3& InHalfZ, const Vec4& InColor);

    void SortForPainter();

    static void BuildPlaneBasis(const Vec3& InAxis, Vec3& OutU, Vec3& OutV);

private:
    uint32_t AddVertex(const Vec3& InPosition, const Vec4& InColor);
    void AddShadedQuad(const Vec3& InA, const Vec3& InB, const Vec3& InC, const Vec3& InD, const Vec4& InColor);
    Vec4 Shade(const Vec4& InColor, const Vec3& InNormal) const;
    Vec3 DirectionToViewer(const Vec3& InPoint) const;
    Vec3 SideOffset(const Vec3& InPoint, const Vec3& InTangent, float InHalfWidth) const;

    Array<GizmoVertex> m_Vertices;
    Array<uint32_t> m_Indices;
    Vec3 m_EyePosition = Vec3(0.0f);
    Vec3 m_ViewDirection = Vec3(0.0f, 0.0f, 1.0f);
    bool m_Orthographic = false;
};
