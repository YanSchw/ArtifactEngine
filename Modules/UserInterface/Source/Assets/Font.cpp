#include "Font.h"
#include "Rendering/Texture.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Platform/FileIO.h"
#include "Common/ByteString.h"
#include "Serialization/ChunkedBinary.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "msdfgen/msdfgen.h"

#include <cmath>

// MTSDF atlas parameters. Each glyph is stored as a multi-channel + true signed distance field:
// RGB holds the multi-channel distance (which preserves sharp corners) and A holds a plain SDF
// (kept for outline/shadow effects). The text shader takes the median of RGB and antialiases the
// 0.5 iso-line. s_MsdfPxRange is the distance range in atlas texels and MUST match the pxRange
// constant in UIText.glsl. s_MsdfPadding must be at least half the range so the field is not
// clipped at a glyph cell's border.
static const uint32_t s_AtlasWidth = 1024;
static const uint32_t s_AtlasHeight = 1024;
static const int s_MsdfPadding = 4;
static const double s_MsdfPxRange = 4.0;
static const double s_MsdfAngleThreshold = 3.0;
static const uint32_t s_FirstCodepoint = 32;
static const uint32_t s_LastCodepoint = 126;

Font::Font() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

void Font::Load() {
#if defined(AE_PACKAGED)
    uint32_t atlasWidth = 0, atlasHeight = 0;
    int32_t glyphCount = 0;
    {
        ChunkReader chunkReader = GetChunkedBinary()->GetChunk(1);
        chunkReader >> atlasWidth;
        chunkReader >> atlasHeight;
        chunkReader >> m_BasePixelHeight;
        chunkReader >> m_AscentPx;
        chunkReader >> m_DescentPx;
        chunkReader >> m_LineGapPx;
        chunkReader >> glyphCount;
    }
    {
        ChunkReader chunkReader = GetChunkedBinary()->GetChunk(2);
        for (int32_t i = 0; i < glyphCount; i++) {
            uint32_t codepoint = 0;
            GlyphInfo glyph;
            chunkReader >> codepoint;
            chunkReader >> glyph;
            m_Glyphs[codepoint] = glyph;
        }
    }
    {
        ChunkReader chunkReader = GetChunkedBinary()->GetChunk(3);
        Array<byte> atlas;
        atlas.Resize((size_t)atlasWidth * atlasHeight * 4);
        chunkReader.ReadBytes(atlas.Data(), atlas.Size());
        CreateAtlasTexture(atlas.Data(), atlasWidth, atlasHeight);
    }
#else
    const String path = EngineConfig::ResolveContentPath(m_FontPath);
    SharedObjectPtr<ByteString> bytes = FileIO::ReadFileToBytes(path);
    if (!bytes || bytes->GetSizeInBytes() == 0) {
        AE_ERROR("Font::Load failed to read font file '{0}'", path);
        return;
    }

    Array<byte> atlas;
    if (BuildAtlasData(bytes->GetData(), atlas)) {
        CreateAtlasTexture(atlas.Data(), s_AtlasWidth, s_AtlasHeight);
    }
#endif
}

// Translates one glyph's stb_truetype outline (font units, Y-up) into an msdfgen::Shape.
// stb emits each contour as a move followed by line/quadratic/cubic segments; contours are
// closed with an explicit edge back to the start when the outline does not already return there.
static void BuildGlyphShape(const stbtt_fontinfo& InFont, int InCodepoint, msdfgen::Shape& OutShape) {
    stbtt_vertex* vertices = nullptr;
    const int vertexCount = stbtt_GetCodepointShape(&InFont, InCodepoint, &vertices);
    if (vertexCount <= 0 || !vertices) {
        if (vertices) {
            stbtt_FreeShape(&InFont, vertices);
        }
        return;
    }

    msdfgen::Contour* contour = nullptr;
    msdfgen::Point2 contourStart(0.0, 0.0);
    msdfgen::Point2 pen(0.0, 0.0);

    for (int i = 0; i < vertexCount; i++) {
        const stbtt_vertex& v = vertices[i];
        const msdfgen::Point2 to((double)v.x, (double)v.y);
        switch (v.type) {
            case STBTT_vmove:
                if (contour && pen != contourStart) {
                    contour->addEdge(msdfgen::EdgeHolder(pen, contourStart));
                }
                contour = &OutShape.addContour();
                contourStart = to;
                break;
            case STBTT_vline:
                if (contour && to != pen) {
                    contour->addEdge(msdfgen::EdgeHolder(pen, to));
                }
                break;
            case STBTT_vcurve:
                if (contour) {
                    contour->addEdge(msdfgen::EdgeHolder(pen, msdfgen::Point2((double)v.cx, (double)v.cy), to));
                }
                break;
            case STBTT_vcubic:
                if (contour) {
                    contour->addEdge(msdfgen::EdgeHolder(pen, msdfgen::Point2((double)v.cx, (double)v.cy), msdfgen::Point2((double)v.cx1, (double)v.cy1), to));
                }
                break;
        }
        pen = to;
    }

    if (contour && pen != contourStart) {
        contour->addEdge(msdfgen::EdgeHolder(pen, contourStart));
    }

    stbtt_FreeShape(&InFont, vertices);
}

bool Font::BuildAtlasData(const byte* InTtfData, Array<byte>& OutAtlasPixels) {
    stbtt_fontinfo font;
    const int offset = stbtt_GetFontOffsetForIndex(InTtfData, 0);
    if (!stbtt_InitFont(&font, InTtfData, offset)) {
        AE_ERROR("Font::BuildAtlasData failed to initialize stb_truetype font");
        return false;
    }

    const float scale = stbtt_ScaleForPixelHeight(&font, m_BasePixelHeight);

    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    m_AscentPx = (float)ascent * scale;
    m_DescentPx = (float)descent * scale; // negative
    m_LineGapPx = (float)lineGap * scale;

    Array<byte>& atlas = OutAtlasPixels;
    atlas.Resize((size_t)s_AtlasWidth * s_AtlasHeight * 4);
    memset(atlas.Data(), 0, atlas.Size());

    uint32_t penX = 0, penY = 0, rowHeight = 0;

    for (uint32_t cp = s_FirstCodepoint; cp <= s_LastCodepoint; cp++) {
        int advance = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(&font, (int)cp, &advance, &lsb);

        GlyphInfo glyph;
        glyph.AdvancePx = (float)advance * scale;

        msdfgen::Shape shape;
        BuildGlyphShape(font, (int)cp, shape);
        if (shape.contours.empty()) {
            m_Glyphs[cp] = glyph; // whitespace and other outline-less glyphs
            continue;
        }

        shape.normalize();
        shape.orientContours();
        msdfgen::edgeColoringSimple(shape, s_MsdfAngleThreshold);

        const msdfgen::Shape::Bounds bounds = shape.getBounds();
        const double glyphWidthPx = (bounds.r - bounds.l) * scale;
        const double glyphHeightPx = (bounds.t - bounds.b) * scale;
        if (glyphWidthPx <= 0.0 || glyphHeightPx <= 0.0) {
            m_Glyphs[cp] = glyph;
            continue;
        }

        const int w = (int)std::ceil(glyphWidthPx) + 2 * s_MsdfPadding;
        const int h = (int)std::ceil(glyphHeightPx) + 2 * s_MsdfPadding;

        // Advance to the next shelf row if the glyph doesn't fit on the current one.
        if (penX + (uint32_t)w + 1 > s_AtlasWidth) {
            penX = 0;
            penY += rowHeight + 1;
            rowHeight = 0;
        }
        if (penY + (uint32_t)h > s_AtlasHeight) {
            AE_WARN("Font::BuildAtlasData ran out of atlas space at codepoint {0}", cp);
            m_Glyphs[cp] = glyph;
            continue;
        }

        // Map shape coordinates (font units, Y-up) to the glyph cell so the shape's lower-left
        // bound lands at (padding, padding); the range is expressed in shape units.
        const msdfgen::Projection projection(
            msdfgen::Vector2(scale, scale),
            msdfgen::Vector2((double)s_MsdfPadding / scale - bounds.l, (double)s_MsdfPadding / scale - bounds.b));
        const msdfgen::Range range = msdfgen::Range(s_MsdfPxRange) / (double)scale;

        msdfgen::Bitmap<float, 4> bitmap(w, h);
        msdfgen::generateMTSDF(bitmap, shape, projection, range, msdfgen::MSDFGeneratorConfig());

        // msdfgen stores row 0 at the bottom (Y-up); the atlas is top-down, so rows are flipped.
        const msdfgen::BitmapSection<float, 4> section = bitmap;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                const float* src = section(x, h - 1 - y);
                const uint32_t ax = penX + (uint32_t)x;
                const uint32_t ay = penY + (uint32_t)y;
                const size_t dst = ((size_t)ay * s_AtlasWidth + ax) * 4;
                atlas[(int)(dst + 0)] = msdfgen::pixelFloatToByte(src[0]);
                atlas[(int)(dst + 1)] = msdfgen::pixelFloatToByte(src[1]);
                atlas[(int)(dst + 2)] = msdfgen::pixelFloatToByte(src[2]);
                atlas[(int)(dst + 3)] = msdfgen::pixelFloatToByte(src[3]);
            }
        }

        glyph.UvMin = Vec2((float)penX / s_AtlasWidth, (float)penY / s_AtlasHeight);
        glyph.UvMax = Vec2((float)(penX + w) / s_AtlasWidth, (float)(penY + h) / s_AtlasHeight);
        glyph.SizePx = Vec2((float)w, (float)h);
        glyph.OffsetPx = Vec2(
            (float)(bounds.l * scale) - (float)s_MsdfPadding,
            (float)s_MsdfPadding - (float)(bounds.b * scale) - (float)h);

        penX += (uint32_t)w + 1;
        rowHeight = std::max(rowHeight, (uint32_t)h);
        m_Glyphs[cp] = glyph;
    }

    return true;
}

void Font::CreateAtlasTexture(byte* InPixels, uint32_t InWidth, uint32_t InHeight) {
    TextureDesc desc;
    desc.Width = InWidth;
    desc.Height = InHeight;
    desc.Format = ImageFormat::RGBA8;
    desc.Usage = ImageUsage::Sampled;
    desc.GenerateMips = false;
    m_AtlasTexture = Texture::Create(InPixels, InWidth, InHeight, 4, desc);
}

bool Font::GetGlyph(uint32_t InCodepoint, GlyphInfo& OutGlyph) {
    if (!m_Glyphs.ContainsKey(InCodepoint)) {
        return false;
    }
    OutGlyph = m_Glyphs.At(InCodepoint);
    return true;
}

Texture* Font::GetAtlasTexture() const {
    return m_AtlasTexture.Get();
}

Vec2 Font::MeasureText(const String& InText, float InPixelHeight) {
    const float scale = GetScaleForPixelHeight(InPixelHeight);
    float width = 0.0f;
    for (char c : InText) {
        GlyphInfo glyph;
        if (GetGlyph((uint32_t)(unsigned char)c, glyph)) {
            width += glyph.AdvancePx * scale;
        }
    }
    return Vec2(width, GetLineHeightBasePx() * scale);
}

float Font::GetTextWidth(const String& InText, float InPixelHeight) {
    const float scale = GetScaleForPixelHeight(InPixelHeight);
    float width = 0.0f;
    for (char c : InText) {
        GlyphInfo glyph;
        if (GetGlyph((uint32_t)(unsigned char)c, glyph)) {
            width += glyph.AdvancePx * scale;
        }
    }
    return width;
}

void Font::Unload() {
    m_AtlasTexture = nullptr;
    m_Glyphs.Clear();
}

void Font::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    const String path = EngineConfig::ResolveContentPath(m_FontPath);
    SharedObjectPtr<ByteString> bytes = FileIO::ReadFileToBytes(path);
    if (!bytes || bytes->GetSizeInBytes() == 0) {
        AE_ERROR("Font::Cook failed to read font file '{0}'", path);
        return;
    }

    Array<byte> atlas;
    if (!BuildAtlasData(bytes->GetData(), atlas)) {
        return;
    }

    {
        ChunkWriter chunkWriter;
        chunkWriter << s_AtlasWidth;
        chunkWriter << s_AtlasHeight;
        chunkWriter << m_BasePixelHeight;
        chunkWriter << m_AscentPx;
        chunkWriter << m_DescentPx;
        chunkWriter << m_LineGapPx;
        chunkWriter << (int32_t)m_Glyphs.Size();
        OutChunkedBinary.AddChunk(1, chunkWriter);
    }
    {
        ChunkWriter chunkWriter;
        for (const auto& [codepoint, glyph] : m_Glyphs) {
            chunkWriter << codepoint;
            chunkWriter << glyph;
        }
        OutChunkedBinary.AddChunk(2, chunkWriter);
    }
    {
        ChunkWriter chunkWriter;
        chunkWriter.WriteBytes(atlas.Data(), atlas.Size());
        OutChunkedBinary.AddChunk(3, chunkWriter);
    }
}

bool Font::IsLoaded() const {
    return Super::IsLoaded() && m_AtlasTexture.Get() != nullptr;
}
