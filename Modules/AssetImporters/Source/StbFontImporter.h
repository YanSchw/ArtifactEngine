#pragma once
#include "Serialization/Assets/FontImporter.h"
#include "StbFontImporter.gen.h"

class StbFontImporter : public FontImporter {
public:
    ARTIFACT_CLASS();

    virtual bool CanImport(const String& InExtension) const override;
    virtual bool Import(const String& InFilePath, const Array<uint32_t>& InCodepoints, ImportedFont& OutFont) override;
};
