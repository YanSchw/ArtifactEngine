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