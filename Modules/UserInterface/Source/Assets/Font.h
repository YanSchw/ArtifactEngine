#pragma once
#include "Assets/Asset.h"
#include "Object/Pointer.h"
#include "Common/Types.h"
#include "Common/Map.h"
#include "Font.gen.h"

class Texture;

/** Metrics for a single glyph in the SDF atlas, expressed in the atlas's base pixel size.
 *  Scale by GetScaleForPixelHeight(targetPx) to draw at another size. */
struct GlyphInfo {
    Vec2 UvMin = Vec2(0.0f);   // atlas UV (0..1) of the glyph quad's top-left
    Vec2 UvMax = Vec2(0.0f);   // atlas UV (0..1) of the glyph quad's bottom-right
    Vec2 SizePx = Vec2(0.0f);  // glyph quad size in base pixels
    Vec2 OffsetPx = Vec2(0.0f);// offset from the baseline pen origin to the quad top-left (xoff, yoff)
    float AdvancePx = 0.0f;    // horizontal advance in base pixels
};

/** A font asset. On load it rasterizes an SDF atlas for the printable
 *  ASCII range using stb_truetype and uploads it as a single RGBA8 Texture (white RGB, SDF in
 *  alpha). */
class Font : public Asset {
public:
    ARTIFACT_CLASS();
    Font();
    virtual ~Font() = default;

    /** Look up a glyph. Returns false for codepoints not in the atlas. */
    bool GetGlyph(uint32_t InCodepoint, GlyphInfo& OutGlyph);

    Texture* GetAtlasTexture() const;

    float GetScaleForPixelHeight(float InPixelHeight) const { return InPixelHeight / m_BasePixelHeight; }
    float GetAscentBasePx() const { return m_AscentPx; }
    float GetDescentBasePx() const { return m_DescentPx; }
    float GetLineHeightBasePx() const { return m_AscentPx - m_DescentPx + m_LineGapPx; }

    /** Measure a run of text at the given pixel height: x = total advance, y = line height. */
    Vec2 MeasureText(const String& InText, float InPixelHeight);

protected:
    virtual void Load() override;
    virtual void Unload() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

public:
    virtual bool IsLoaded() const override;

private:
    void BuildAtlas(const byte* InTtfData);

    PROPERTY()
    String m_FontPath;

    SharedObjectPtr<Texture> m_AtlasTexture = nullptr;
    Map<uint32_t, GlyphInfo> m_Glyphs;

    float m_BasePixelHeight = 48.0f;
    float m_AscentPx = 0.0f;
    float m_DescentPx = 0.0f;
    float m_LineGapPx = 0.0f;
};
