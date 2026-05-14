#pragma once
#include "CoreMinimal.h"
#include "Common/Types.h"
#include "Image.h"
#include "FrameBuffer.gen.h"

struct FrameBufferDesc {
    ARTIFACT_STRUCT();

    PROPERTY()
    uint32_t Width = 1;

    PROPERTY()
    uint32_t Height = 1;

    PROPERTY()
    Array<SharedObjectPtr<ImageView>> ColorAttachments;
    
    PROPERTY()
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