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

    /** Multisample count every attachment is rendered with. */
    SampleCount Samples = SampleCount::None;

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

    SampleCount GetSamples() const { return m_Desc.Samples; }
    bool IsMultisampled() const { return ::IsMultisampled(m_Desc.Samples); }

    /** Reads back one texel of an R32UI attachment. Stalls the GPU! */
    virtual uint32_t ReadPixelUint(int32_t InAttachment, uint32_t InX, uint32_t InY) const = 0;

    static SharedObjectPtr<FrameBuffer> Create(const FrameBufferDesc& InFrameBufferDesc);

    static SampleCount GetMaxSupportedSampleCount();
protected:
    FrameBufferDesc m_Desc;
};