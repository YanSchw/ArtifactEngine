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
    R32UI,

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
    DepthStencil    = 1 << 5,
    Transient       = 1 << 6
};

ARTIFACT_ENUM();
enum class SampleCount : uint32_t {
    None = 1,
    X2   = 2,
    X4   = 4,
    X8   = 8
};

inline bool IsMultisampled(SampleCount InSamples) { return InSamples != SampleCount::None; }

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

    /** Anything but None makes this a multisampled image. Multisampled images can only be used as
     *  render pass attachments - they cannot be sampled, copied or read back until resolved. */
    SampleCount Samples = SampleCount::None;

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
    SharedObjectPtr<Image> ImagePtr;

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