#pragma once
#include "CoreMinimal.h"
#include "Rendering/Image.h"
#include "Rendering/Sampler.h"
#include "Rendering/ShaderDataType.h"
#include "Rendering/ShaderSource.h"

#include <GLES3/gl3.h>

struct WebGLFormat {
    GLenum InternalFormat = GL_RGBA8;
    GLenum Format = GL_RGBA;
    GLenum Type = GL_UNSIGNED_BYTE;
    GLenum Attachment = GL_COLOR_ATTACHMENT0;
};

WebGLFormat GetWebGLFormat(ImageFormat InFormat);
bool IsDepthFormat(ImageFormat InFormat);

GLenum GetWebGLMinFilter(FilterMode InFilter, bool InMipmapped);
GLenum GetWebGLMagFilter(FilterMode InFilter);
GLenum GetWebGLAddressMode(AddressMode InMode);
GLenum GetWebGLCompareOp(CompareOp InCompare);

void ApplyRenderState(const ShaderRenderState& InState);

uint32_t GetComponentCount(ShaderDataType InType);
uint32_t GetTypeSize(ShaderDataType InType);
