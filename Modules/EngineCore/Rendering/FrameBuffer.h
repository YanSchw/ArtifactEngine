#pragma once
#include "Core/Object.h"
#include "Core/Pointer.h"
#include "Common/Types.h"
#include "Image.h"
#include "FrameBuffer.gen.h"

struct FrameBufferDesc {
    uint32_t Width = 1;
    uint32_t Height = 1;

    Array<SharedObjectPtr<ImageView>> ColorAttachments;
    SharedObjectPtr<ImageView> DepthAttachment;
};

class FrameBuffer : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~FrameBuffer() = default;

    const FrameBufferDesc& GetDesc() const { return m_Desc; }

    static SharedObjectPtr<FrameBuffer> Create(const FrameBufferDesc& InFrameBufferDesc);
protected:
    FrameBufferDesc m_Desc;
};