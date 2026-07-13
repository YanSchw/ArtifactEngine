#include "VectorImage.h"
#include "Core/EngineConfig.h"
#include "Platform/FileIO.h"
#include "Serialization/ChunkedBinary.h"

VectorImage::VectorImage() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

void VectorImage::Load() {
#if defined(AE_PACKAGED)
    ChunkReader chunkReader = GetChunkedBinary()->GetChunk(1);
    chunkReader >> m_Document.Width;
    chunkReader >> m_Document.Height;
    int32_t shapeCount;
    chunkReader >> shapeCount;
    for (int32_t s = 0; s < shapeCount; s++) {
        SvgShape shape;
        uint8_t hasFill, fillRule, hasGradient, hasStroke, cap, join;
        chunkReader >> hasFill;
        shape.HasFill = hasFill != 0;
        chunkReader >> shape.FillColor;
        chunkReader >> fillRule;
        shape.FillRule = (SvgFillRule)fillRule;
        chunkReader >> hasGradient;
        shape.HasFillGradient = hasGradient != 0;
        if (shape.HasFillGradient) {
            chunkReader >> shape.FillGradient.TCoeffs;
            int32_t stopCount;
            chunkReader >> stopCount;
            shape.FillGradient.Stops.Resize(stopCount);
            chunkReader.ReadBytes(&shape.FillGradient.Stops[0], sizeof(SvgGradientStop) * stopCount);
        }
        chunkReader >> hasStroke;
        shape.HasStroke = hasStroke != 0;
        chunkReader >> shape.StrokeColor;
        chunkReader >> shape.StrokeWidth;
        chunkReader >> cap;
        shape.Cap = (SvgLineCap)cap;
        chunkReader >> join;
        shape.Join = (SvgLineJoin)join;
        chunkReader >> shape.MiterLimit;
        int32_t contourCount;
        chunkReader >> contourCount;
        for (int32_t c = 0; c < contourCount; c++) {
            SvgContour contour;
            uint8_t closed;
            chunkReader >> closed;
            contour.Closed = closed != 0;
            int32_t pointCount;
            chunkReader >> pointCount;
            contour.Points.Resize(pointCount);
            chunkReader.ReadBytes(&contour.Points[0], sizeof(Vec2) * pointCount);
            shape.Contours.Add(std::move(contour));
        }
        m_Document.Shapes.Add(std::move(shape));
    }
    m_Parsed = true;
#else
    String path = EngineConfig::ResolveContentPath(m_ImagePath);
    m_Parsed = SvgImporter::Parse(FileIO::ReadFileToString(path), m_Document);
    if (!m_Parsed) {
        AE_WARN("Failed to import SVG {0}", path);
    }
#endif
}

void VectorImage::Unload() {
    m_Document = SvgDocument();
    m_Parsed = false;
}

void VectorImage::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    String path = EngineConfig::ResolveContentPath(m_ImagePath);
    SvgDocument document;
    if (!SvgImporter::Parse(FileIO::ReadFileToString(path), document)) {
        AE_WARN("Failed to import SVG {0} while cooking", path);
    }

    ChunkWriter chunkWriter;
    chunkWriter << document.Width;
    chunkWriter << document.Height;
    chunkWriter << (int32_t)document.Shapes.Size();
    for (const SvgShape& shape : document.Shapes) {
        chunkWriter << (uint8_t)(shape.HasFill ? 1 : 0);
        chunkWriter << shape.FillColor;
        chunkWriter << (uint8_t)shape.FillRule;
        chunkWriter << (uint8_t)(shape.HasFillGradient ? 1 : 0);
        if (shape.HasFillGradient) {
            chunkWriter << shape.FillGradient.TCoeffs;
            chunkWriter << (int32_t)shape.FillGradient.Stops.Size();
            chunkWriter.WriteBytes(&shape.FillGradient.Stops[0], sizeof(SvgGradientStop) * shape.FillGradient.Stops.Size());
        }
        chunkWriter << (uint8_t)(shape.HasStroke ? 1 : 0);
        chunkWriter << shape.StrokeColor;
        chunkWriter << shape.StrokeWidth;
        chunkWriter << (uint8_t)shape.Cap;
        chunkWriter << (uint8_t)shape.Join;
        chunkWriter << shape.MiterLimit;
        chunkWriter << (int32_t)shape.Contours.Size();
        for (const SvgContour& contour : shape.Contours) {
            chunkWriter << (uint8_t)(contour.Closed ? 1 : 0);
            chunkWriter << (int32_t)contour.Points.Size();
            chunkWriter.WriteBytes(&contour.Points[0], sizeof(Vec2) * contour.Points.Size());
        }
    }
    OutChunkedBinary.AddChunk(1, chunkWriter);
}

bool VectorImage::IsLoaded() const {
    return Super::IsLoaded() && m_Parsed;
}

void VectorImage::Tessellate(float InScale, SvgMesh& OutMesh) const {
    SvgTessellator::Tessellate(m_Document, InScale, OutMesh);
}
