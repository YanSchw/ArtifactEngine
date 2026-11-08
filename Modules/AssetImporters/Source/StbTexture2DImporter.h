#pragma once
#include "Serialization/Assets/Texture2DImporter.h"
#include "StbTexture2DImporter.gen.h"

class StbTexture2DImporter : public Texture2DImporter {
public:
    ARTIFACT_CLASS();

    virtual bool CanImport(const String& InExtension) const override;
    virtual bool Import(const String& InFilePath, Array<byte>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight) override;
    virtual bool IsHighDynamicRange(const String& InFilePath) const override;
    virtual bool ImportHighDynamicRange(const String& InFilePath, Array<float>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight) override;
};
