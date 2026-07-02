#include "Font.h"
#include "Rendering/Texture.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Platform/FileIO.h"
#include "Common/ByteString.h"
#include "Serialization/ChunkedBinary.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// SDF atlas parameters. The atlas stores white RGB with the signed distance in the alpha channel;
// the text shader antialiases the 0.5 iso-line. onedge_value 128 => the edge sits at alpha ~0.5.
static const uint32_t s_AtlasWidth = 512;
static const uint32_t s_AtlasHeight = 512;
static const int s_SdfPadding = 4;
static const unsigned char s_SdfOnEdge = 128;
static const float s_SdfPixelDistScale = 128.0f / (float)s_SdfPadding;
static const uint32_t s_FirstCodepoint = 32;
static const uint32_t s_LastCodepoint = 126;

Font::Font() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

void Font::Load() {
    const String path = EngineConfig::ContentDir() + m_FontPath;
    SharedObjectPtr<ByteString> bytes = FileIO::ReadFileToBytes(path);
    if (!bytes || bytes->GetSizeInBytes() == 0) {
        AE_ERROR("Font::Load failed to read font file '{0}'", path);
        return;
    }

    BuildAtlas(bytes->GetData());
}

void Font::BuildAtlas(const byte* InTtfData) {
    stbtt_fontinfo font;
    const int offset = stbtt_GetFontOffsetForIndex(InTtfData, 0);
    if (!stbtt_InitFont(&font, InTtfData, offset)) {
        AE_ERROR("Font::BuildAtlas failed to initialize stb_truetype font");
        return;
    }

    const float scale = stbtt_ScaleForPixelHeight(&font, m_BasePixelHeight);

    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    m_AscentPx = (float)ascent * scale;
    m_DescentPx = (float)descent * scale; // negative
    m_LineGapPx = (float)lineGap * scale;

    // Transparent RGBA atlas; glyphs write white RGB + SDF alpha.
    Array<byte> atlas;
    atlas.Resize((size_t)s_AtlasWidth * s_AtlasHeight * 4);
    memset(atlas.Data(), 0, atlas.Size());

    uint32_t penX = 0, penY = 0, rowHeight = 0;

    for (uint32_t cp = s_FirstCodepoint; cp <= s_LastCodepoint; cp++) {
        int advance = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(&font, (int)cp, &advance, &lsb);

        GlyphInfo glyph;
        glyph.AdvancePx = (float)advance * scale;

        int w = 0, h = 0, xoff = 0, yoff = 0;
        unsigned char* sdf = stbtt_GetCodepointSDF(&font, scale, (int)cp, s_SdfPadding, s_SdfOnEdge, s_SdfPixelDistScale, &w, &h, &xoff, &yoff);

        if (sdf && w > 0 && h > 0) {
            // Advance to the next shelf row if the glyph doesn't fit on the current one.
            if (penX + (uint32_t)w + 1 > s_AtlasWidth) {
                penX = 0;
                penY += rowHeight + 1;
                rowHeight = 0;
            }

            if (penY + (uint32_t)h <= s_AtlasHeight) {
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        const uint32_t ax = penX + (uint32_t)x;
                        const uint32_t ay = penY + (uint32_t)y;
                        const size_t dst = ((size_t)ay * s_AtlasWidth + ax) * 4;
                        atlas[(int)(dst + 0)] = 255;
                        atlas[(int)(dst + 1)] = 255;
                        atlas[(int)(dst + 2)] = 255;
                        atlas[(int)(dst + 3)] = sdf[y * w + x];
                    }
                }

                glyph.UvMin = Vec2((float)penX / s_AtlasWidth, (float)penY / s_AtlasHeight);
                glyph.UvMax = Vec2((float)(penX + w) / s_AtlasWidth, (float)(penY + h) / s_AtlasHeight);
                glyph.SizePx = Vec2((float)w, (float)h);
                glyph.OffsetPx = Vec2((float)xoff, (float)yoff);

                penX += (uint32_t)w + 1;
                rowHeight = std::max(rowHeight, (uint32_t)h);
            } else {
                AE_WARN("Font::BuildAtlas ran out of atlas space at codepoint {0}", cp);
            }
        }

        if (sdf) {
            stbtt_FreeSDF(sdf, nullptr);
        }

        m_Glyphs[cp] = glyph;
    }

    TextureDesc desc;
    desc.Width = s_AtlasWidth;
    desc.Height = s_AtlasHeight;
    desc.Format = ImageFormat::RGBA8;
    desc.Usage = ImageUsage::Sampled;
    desc.GenerateMips = false;
    m_AtlasTexture = Texture::Create(atlas.Data(), s_AtlasWidth, s_AtlasHeight, 4, desc);
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

void Font::Unload() {
    m_AtlasTexture = nullptr;
    m_Glyphs.Clear();
}

void Font::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);
}

bool Font::IsLoaded() const {
    return Super::IsLoaded() && m_AtlasTexture.Get() != nullptr;
}
