#pragma once
#include "CoreMinimal.h"
#include "Common/Types.h"
#include "Image.h"
#include "Texture.gen.h"

enum class TextureType {
    Texture2D,
    TextureCube
};

struct TextureDesc {
    TextureType Type = TextureType::Texture2D;

    uint32_t Width = 1;
    uint32_t Height = 1;

    ImageFormat Format = ImageFormat::RGBA8;
    ImageUsage Usage = ImageUsage::Sampled;

    bool GenerateMips = true;
};

class Texture : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~Texture() = default;

    virtual SharedObjectPtr<Image> GetImage() const = 0;
    virtual SharedObjectPtr<ImageView> GetDefaultView() const = 0;

    static SharedObjectPtr<Texture> Create(const String& InFilePath, const TextureDesc& InTextureDesc = TextureDesc());
};