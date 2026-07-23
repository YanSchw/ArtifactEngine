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

// SDF atlas parameters. Each glyph is stored as a single-channel signed distance field replicated
// across RGBA; the text shader takes median(RGB) (which equals the value when the channels match)
// and antialiases the 0.5 iso-line. The field is built from a *filled* nonzero-winding
// rasterization rather than the glyph outline, so fonts whose glyphs are drawn with overlapping or
// self-intersecting contours (e.g. Inter, whose 'e' is one self-overlapping contour) don't get the
// seam/gash artifacts that edge-based MSDF/SDF generation produces on them — edges internal to the
// overlap simply don't exist in the fill. s_SdfPxRange is the distance range in atlas texels and
// MUST match the pxRange constant in UIText.glsl. s_SdfPadding must be at least half the range so
// the field is not clipped at a glyph cell's border. s_SdfSupersample is the fill-resolution
// multiple used before the distance transform (higher = finer sub-texel edges).
static const uint32_t s_AtlasWidth = 1024;
static const uint32_t s_AtlasHeight = 1024;
static const int s_SdfPadding = 4;
static const double s_SdfPxRange = 4.0;
static const int s_SdfSupersample = 6;
static const float s_EdtInf = 1e20f;
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

// Felzenszwalb & Huttenlocher exact Euclidean distance transform (returns squared distance to the
// nearest seed, where seeds are the entries with value 0 and everything else starts at s_EdtInf).
// InV/InZ are caller-provided scratch of length InN and InN+1 respectively.
static void SdfEdt1D(const float* InF, float* OutD, int* InV, float* InZ, int InN) {
    int k = 0;
    InV[0] = 0;
    InZ[0] = -s_EdtInf;
    InZ[1] = s_EdtInf;
    for (int q = 1; q < InN; q++) {
        float s = ((InF[q] + (float)q * q) - (InF[InV[k]] + (float)InV[k] * InV[k])) / (float)(2 * q - 2 * InV[k]);
        while (s <= InZ[k]) {
            k--;
            s = ((InF[q] + (float)q * q) - (InF[InV[k]] + (float)InV[k] * InV[k])) / (float)(2 * q - 2 * InV[k]);
        }
        k++;
        InV[k] = q;
        InZ[k] = s;
        InZ[k + 1] = s_EdtInf;
    }
    k = 0;
    for (int q = 0; q < InN; q++) {
        while (InZ[k + 1] < (float)q) {
            k++;
        }
        const float d = (float)(q - InV[k]);
        OutD[q] = d * d + InF[InV[k]];
    }
}

// In-place squared Euclidean distance transform of a 2D grid: transform every column, then every row.
static void SdfEdt2D(float* InoutGrid, int InW, int InH) {
    const int maxDim = InW > InH ? InW : InH;
    Array<float> f;
    Array<float> d;
    Array<float> z;
    Array<int> v;
    f.Resize((size_t)maxDim);
    d.Resize((size_t)maxDim);
    z.Resize((size_t)maxDim + 1);
    v.Resize((size_t)maxDim);
    for (int x = 0; x < InW; x++) {
        for (int y = 0; y < InH; y++) {
            f[y] = InoutGrid[y * InW + x];
        }
        SdfEdt1D(f.Data(), d.Data(), v.Data(), z.Data(), InH);
        for (int y = 0; y < InH; y++) {
            InoutGrid[y * InW + x] = d[y];
        }
    }
    for (int y = 0; y < InH; y++) {
        for (int x = 0; x < InW; x++) {
            f[x] = InoutGrid[y * InW + x];
        }
        SdfEdt1D(f.Data(), d.Data(), v.Data(), z.Data(), InW);
        for (int x = 0; x < InW; x++) {
            InoutGrid[y * InW + x] = d[x];
        }
    }
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

        const msdfgen::Shape::Bounds bounds = shape.getBounds();
        const double glyphWidthPx = (bounds.r - bounds.l) * scale;
        const double glyphHeightPx = (bounds.t - bounds.b) * scale;
        if (glyphWidthPx <= 0.0 || glyphHeightPx <= 0.0) {
            m_Glyphs[cp] = glyph;
            continue;
        }

        const int w = (int)std::ceil(glyphWidthPx) + 2 * s_SdfPadding;
        const int h = (int)std::ceil(glyphHeightPx) + 2 * s_SdfPadding;

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

        // Rasterize the glyph as a filled nonzero-winding mask at s_SdfSupersample x the cell
        // resolution, mapping the shape's lower-left bound to the cell's (padding, padding). Because
        // this is a fill, overlapping/self-intersecting contours collapse into one solid region with
        // no internal edges, which is what keeps the resulting field free of overlap seams.
        const int ss = s_SdfSupersample;
        const msdfgen::Projection projection(
            msdfgen::Vector2(scale * ss, scale * ss),
            msdfgen::Vector2((double)s_SdfPadding / scale - bounds.l, (double)s_SdfPadding / scale - bounds.b));
        const int fillW = w * ss;
        const int fillH = h * ss;
        msdfgen::Bitmap<float, 1> coverage(fillW, fillH);
        msdfgen::rasterize(coverage, shape, projection, msdfgen::FILL_NONZERO);

        // Signed Euclidean distance transform: distIn = squared distance from inside texels to the
        // nearest outside texel, distOut the reverse; combined they give a smooth signed distance.
        const msdfgen::BitmapConstRef<float, 1> fill = coverage;
        Array<float> distIn;
        Array<float> distOut;
        distIn.Resize((size_t)fillW * fillH);
        distOut.Resize((size_t)fillW * fillH);
        for (int y = 0; y < fillH; y++) {
            for (int x = 0; x < fillW; x++) {
                const int i = y * fillW + x;
                const bool inside = *fill(x, y) >= 0.5f;
                distIn[i] = inside ? s_EdtInf : 0.0f;
                distOut[i] = inside ? 0.0f : s_EdtInf;
            }
        }
        SdfEdt2D(distIn.Data(), fillW, fillH);
        SdfEdt2D(distOut.Data(), fillW, fillH);

        // One supersampled texel per atlas texel, flipping rows (the fill is Y-up, the atlas is
        // top-down). Encode so 0.5 is the edge and s_SdfPxRange texels span the transition band,
        // matching UIText.glsl; the same value goes to every channel so median(RGB) recovers it.
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                const int fx = x * ss + ss / 2;
                const int fy = (h - 1 - y) * ss + ss / 2;
                const int i = fy * fillW + fx;
                const bool inside = *fill(fx, fy) >= 0.5f;
                const float distField = inside ? (std::sqrt(distIn[i]) - 0.5f) : -(std::sqrt(distOut[i]) - 0.5f);
                const float distTexels = distField / (float)ss;
                float stored = (float)(distTexels / s_SdfPxRange) + 0.5f;
                stored = std::max(0.0f, std::min(1.0f, stored));
                const byte value = msdfgen::pixelFloatToByte(stored);
                const uint32_t ax = penX + (uint32_t)x;
                const uint32_t ay = penY + (uint32_t)y;
                const size_t dst = ((size_t)ay * s_AtlasWidth + ax) * 4;
                atlas[(int)(dst + 0)] = value;
                atlas[(int)(dst + 1)] = value;
                atlas[(int)(dst + 2)] = value;
                atlas[(int)(dst + 3)] = value;
            }
        }

        glyph.UvMin = Vec2((float)penX / s_AtlasWidth, (float)penY / s_AtlasHeight);
        glyph.UvMax = Vec2((float)(penX + w) / s_AtlasWidth, (float)(penY + h) / s_AtlasHeight);
        glyph.SizePx = Vec2((float)w, (float)h);
        glyph.OffsetPx = Vec2(
            (float)(bounds.l * scale) - (float)s_SdfPadding,
            (float)s_SdfPadding - (float)(bounds.b * scale) - (float)h);

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
