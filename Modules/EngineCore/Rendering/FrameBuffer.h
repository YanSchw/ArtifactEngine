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

    Vec4 ClearColor = Vec4(0.1f, 0.1f, 0.1f, 1.0f);

    Array<Vec4> ClearColors;

    Vec4 GetClearColor(int32_t InAttachment) const {
        return InAttachment < ClearColors.Size() ? ClearColors[InAttachment] : ClearColor;
    }
};

class FrameBuffer : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~FrameBuffer() = default;

    const FrameBufferDesc& GetDesc() const { return m_Desc; }

    /** Reads back one texel of an R32UI attachment. Stalls the GPU! */
    virtual uint32_t ReadPixelUint(int32_t InAttachment, uint32_t InX, uint32_t InY) const = 0;

    static SharedObjectPtr<FrameBuffer> Create(const FrameBufferDesc& InFrameBufferDesc);
protected:
    FrameBufferDesc m_Desc;
};