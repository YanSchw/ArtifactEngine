#include "StbFontImporter.h"
#include "Platform/FileIO.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

static void ImportOutline(const stbtt_fontinfo& InFont, int InCodepoint, ImportedGlyph& OutGlyph) {
    stbtt_vertex* vertices = nullptr;
    const int vertexCount = stbtt_GetCodepointShape(&InFont, InCodepoint, &vertices);

    for (int i = 0; i < vertexCount; i++) {
        const stbtt_vertex& vertex = vertices[i];
        const Vec2 to(vertex.x, vertex.y);

        if (vertex.type == STBTT_vmove) {
            OutGlyph.Contours.Add(GlyphContour{ to, {} });
            continue;
        }
        if (OutGlyph.Contours.IsEmpty()) {
            continue;
        }

        GlyphSegment segment;
        if (vertex.type == STBTT_vcurve || vertex.type == STBTT_vcubic) {
            segment.Points.Add(Vec2(vertex.cx, vertex.cy));
        }
        if (vertex.type == STBTT_vcubic) {
            segment.Points.Add(Vec2(vertex.cx1, vertex.cy1));
        }
        segment.Points.Add(to);
        OutGlyph.Contours.LastItem().Segments.Add(segment);
    }

    stbtt_FreeShape(&InFont, vertices);
}

bool StbFontImporter::CanImport(const String& InExtension) const {
    return InExtension == ".ttf" || InExtension == ".otf";
}

bool StbFontImporter::Import(const String& InFilePath, const Array<uint32_t>& InCodepoints, ImportedFont& OutFont) {
    SharedObjectPtr<ByteString> bytes = FileIO::ReadFileToBytes(InFilePath);
    if (!bytes || bytes->GetSizeInBytes() == 0) {
        return false;
    }

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, bytes->GetData(), stbtt_GetFontOffsetForIndex(bytes->GetData(), 0))) {
        return false;
    }

    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    OutFont.Ascent = (float)ascent;
    OutFont.Descent = (float)descent;
    OutFont.LineGap = (float)lineGap;

    for (uint32_t codepoint : InCodepoints) {
        int advance = 0;
        stbtt_GetCodepointHMetrics(&font, (int)codepoint, &advance, nullptr);

        ImportedGlyph& glyph = OutFont.Glyphs[codepoint];
        glyph.Advance = (float)advance;
        ImportOutline(font, (int)codepoint, glyph);
    }
    return true;
}
