#pragma once
#include "CoreMinimal.h"
#include "Common/Types.h"
#include "Image.gen.h"

ARTIFACT_ENUM();
enum class ImageFormat {
    None,

    RGBA8,
    BGRA8,
    RGBA16F,
    RGBA32F,

    Depth24Stencil8,
    Depth32F
};


ARTIFACT_ENUM();
enum class ImageUsage : uint32_t {
    None            = 0,
    TransferSrc     = 1 << 0,
    TransferDst     = 1 << 1,
    Sampled         = 1 << 2,
    Storage         = 1 << 3,
    ColorAttachment = 1 << 4,
    DepthStencil    = 1 << 5
};

ARTIFACT_ENUM();
enum class ImageAspect {
    Color,
    Depth,
    Stencil,
    DepthStencil
};


ARTIFACT_ENUM();
enum class ImageViewType {
    Type2D,
    Type2DArray,
    Cube,
    CubeArray
};

struct ImageDesc {
    uint32_t Width = 1;
    uint32_t Height = 1;
    uint32_t Depth = 1;

    uint32_t MipLevels = 1;
    uint32_t ArrayLayers = 1;

    ImageFormat Format = ImageFormat::RGBA8;
    ImageUsage Usage = ImageUsage::Sampled;

    bool GenerateMips = false;
};

class Image : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~Image() = default;

    const ImageDesc& GetDesc() const { return m_Desc; }

    static SharedObjectPtr<Image> Create(const ImageDesc& InImageDesc);
protected:
    ImageDesc m_Desc;
};

struct ImageViewDesc {
    SharedObjectPtr<Image> Image;

    ImageViewType ViewType = ImageViewType::Type2D;
    ImageFormat Format = ImageFormat::RGBA8;

    uint32_t BaseMip = 0;
    uint32_t MipCount = 1;

    uint32_t BaseLayer = 0;
    uint32_t LayerCount = 1;

    ImageAspect Aspect = ImageAspect::Color;
};

class ImageView : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~ImageView() = default;

    const ImageViewDesc& GetDesc() const { return m_Desc; }

    static SharedObjectPtr<ImageView> Create(const ImageViewDesc& InImageViewDesc);
protected:
    ImageViewDesc m_Desc;
};