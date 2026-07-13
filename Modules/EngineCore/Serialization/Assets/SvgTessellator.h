#pragma once
#include "SvgImporter.h"

struct SvgMesh {
    Array<Vec2> Positions;
    Array<Vec4> Colors;
    Array<uint32_t> Indices;
};

namespace SvgTessellator {
    /** Triangulates the document's filled shapes. InScale is the intended pixels per
     *  document unit and only controls curve flattening precision. */
    void Tessellate(const SvgDocument& InDocument, float InScale, SvgMesh& OutMesh);
}
