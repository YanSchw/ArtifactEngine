#pragma once
#include "Asset.h"
#include "Serialization/Assets/SvgImporter.h"
#include "Serialization/Assets/SvgTessellator.h"
#include "VectorImage.gen.h"

/** A resolution-independent image imported from an SVG file. */
class VectorImage : public Asset {
public:
    ARTIFACT_CLASS();
    VectorImage();
    virtual ~VectorImage() = default;

    /** Document (viewBox) size in SVG units. */
    Vec2 GetSize() const { return Vec2(m_Document.Width, m_Document.Height); }
    const SvgDocument& GetDocument() const { return m_Document; }

    /** InScale = pixels per document unit at the intended draw size; positions in
     *  OutMesh stay in document units. */
    void Tessellate(float InScale, SvgMesh& OutMesh) const;

protected:
    virtual void Load() override;
    virtual void Unload() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

public:
    virtual bool IsLoaded() const override;

private:
    SvgDocument m_Document;
    bool m_Parsed = false;

    PROPERTY()
    String m_ImagePath;
};
