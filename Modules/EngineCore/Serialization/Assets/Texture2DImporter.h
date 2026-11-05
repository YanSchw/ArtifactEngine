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
};
