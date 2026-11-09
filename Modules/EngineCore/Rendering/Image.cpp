#include "Image.h"
#include "RenderingAPI.h"

SharedObjectPtr<Image> Image::Create(const ImageDesc& InImageDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateImage(InImageDesc);
}

SharedObjectPtr<ImageView> ImageView::Create(const ImageViewDesc& InImageViewDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateImageView(InImageViewDesc);
}
SharedObjectPtr<ImageView> ImageView::CreateAttachment(uint32_t InWidth, uint32_t InHeight, ImageFormat InFormat, ImageUsage InUsage) {
    ImageDesc imageDesc;
    imageDesc.Width = InWidth;
    imageDesc.Height = InHeight;
    imageDesc.Format = InFormat;
    imageDesc.Usage = InUsage;

    ImageViewDesc viewDesc;
    viewDesc.ImagePtr = Image::Create(imageDesc);
    viewDesc.Format = InFormat;
    const bool depth = InFormat == ImageFormat::Depth32F || InFormat == ImageFormat::Depth24Stencil8;
    viewDesc.Aspect = depth ? ImageAspect::Depth : ImageAspect::Color;
    return ImageView::Create(viewDesc);
}
