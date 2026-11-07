#pragma once
#include "Image.h"

/** What a Deferred material writes, in output location order, and where the lighting reads it back.
 *  Mirrored by /Shaders/Deferred/Common.glsl. */
struct GBufferLayout {
    static constexpr int32_t Albedo = 0;
    static constexpr int32_t Normal = 1;
    static constexpr int32_t Emissive = 2;
    static constexpr int32_t NodeId = 3;
    static constexpr int32_t AttachmentCount = 4;

    static constexpr ImageFormat Format = ImageFormat::RGBA8;
    static constexpr ImageFormat DepthFormat = ImageFormat::Depth32F;

    /** Attachment i is sampled at FirstBinding + i, the depth right after the last one. */
    static constexpr uint32_t FirstBinding = 3;
    static constexpr uint32_t DepthBinding = FirstBinding + AttachmentCount;
};
