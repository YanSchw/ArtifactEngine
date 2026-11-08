#pragma once
#include "AssetImporter.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "Texture2DImporter.gen.h"

class Texture2DImporter : public AssetImporter {
public:
    ARTIFACT_CLASS();

    /** Decodes the image into tightly packed RGBA8 pixels. */
    virtual bool Import(const String& InFilePath, Array<byte>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight) = 0;

    virtual bool IsHighDynamicRange(const String& InFilePath) const { (void)InFilePath; return false; }

    virtual bool ImportHighDynamicRange(const String& InFilePath, Array<float>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight) {
        (void)InFilePath; (void)OutPixels; (void)OutWidth; (void)OutHeight;
        return false;
    }
};
