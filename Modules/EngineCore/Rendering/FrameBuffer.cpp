#include "FrameBuffer.h"
#include "RenderingAPI.h"

SharedObjectPtr<FrameBuffer> FrameBuffer::Create(const FrameBufferDesc& InFrameBufferDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateFrameBuffer(InFrameBufferDesc);
}
