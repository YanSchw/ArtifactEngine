#include "StbTexture2DImporter.h"

#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

bool StbTexture2DImporter::CanImport(const String& InExtension) const {
    static const Array<String> s_Extensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".psd", ".hdr", ".pic", ".pnm", ".ppm", ".pgm" };
    return s_Extensions.Contains(InExtension);
}

bool StbTexture2DImporter::Import(const String& InFilePath, Array<byte>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(InFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        return false;
    }

    OutWidth = (uint32_t)width;
    OutHeight = (uint32_t)height;
    OutPixels.Resize((size_t)width * height * 4);
    memcpy(OutPixels.Data(), pixels, OutPixels.Size());
    stbi_image_free(pixels);
    return true;
}
