#pragma once
#include "AssetImporter.h"
#include "Common/Array.h"
#include "Common/Map.h"
#include "Common/Types.h"
#include "FontImporter.gen.h"

/** The points after the pen: one for a line, two for a quadratic and three for a cubic bezier. */
struct GlyphSegment {
    Array<Vec2> Points;
};

struct GlyphContour {
    Vec2 Start = Vec2(0.0f);
    Array<GlyphSegment> Segments;
};

struct ImportedGlyph {
    float Advance = 0.0f;
    Array<GlyphContour> Contours;
};

/** Metrics and outlines in font units, Y-up. */
struct ImportedFont {
    float Ascent = 0.0f;
    float Descent = 0.0f;
    float LineGap = 0.0f;
    Map<uint32_t, ImportedGlyph> Glyphs;
};

class FontImporter : public AssetImporter {
public:
    ARTIFACT_CLASS();

    virtual bool Import(const String& InFilePath, const Array<uint32_t>& InCodepoints, ImportedFont& OutFont) = 0;
};
