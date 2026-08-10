#include "WebGLCommon.h"

WebGLFormat GetWebGLFormat(ImageFormat InFormat) {
    switch (InFormat) {
        case ImageFormat::RGBA8:
        // WebGL 2 has no BGRA storage; the engine only asks for it to match a native swapchain.
        case ImageFormat::BGRA8:           return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0 };
        case ImageFormat::RGBA16F:         return { GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, GL_COLOR_ATTACHMENT0 };
        case ImageFormat::RGBA32F:         return { GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0 };
        case ImageFormat::R32UI:           return { GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, GL_COLOR_ATTACHMENT0 };
        case ImageFormat::Depth24Stencil8: return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH_STENCIL_ATTACHMENT };
        case ImageFormat::Depth32F:        return { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT };
        case ImageFormat::None:            break;
    }
    AE_ERROR("Unsupported image format {0}", (uint32_t)InFormat);
    return {};
}

bool IsDepthFormat(ImageFormat InFormat) {
    return InFormat == ImageFormat::Depth24Stencil8 || InFormat == ImageFormat::Depth32F;
}

GLenum GetWebGLMinFilter(FilterMode InFilter, bool InMipmapped) {
    if (InFilter == FilterMode::Nearest) {
        return InMipmapped ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
    }
    return InMipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
}

GLenum GetWebGLMagFilter(FilterMode InFilter) {
    return InFilter == FilterMode::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLenum GetWebGLAddressMode(AddressMode InMode) {
    switch (InMode) {
        case AddressMode::Repeat: return GL_REPEAT;
        case AddressMode::Clamp:  return GL_CLAMP_TO_EDGE;
        case AddressMode::Mirror: return GL_MIRRORED_REPEAT;
    }
    return GL_REPEAT;
}

GLenum GetWebGLCompareOp(CompareOp InCompare) {
    switch (InCompare) {
        case CompareOp::Less:           return GL_LESS;
        case CompareOp::LessOrEqual:    return GL_LEQUAL;
        case CompareOp::Greater:        return GL_GREATER;
        case CompareOp::GreaterOrEqual: return GL_GEQUAL;
        case CompareOp::None:           break;
    }
    return GL_ALWAYS;
}

static void ApplyBlendMode(BlendMode InBlend) {
    if (InBlend == BlendMode::Opaque) {
        glDisable(GL_BLEND);
        return;
    }

    glEnable(GL_BLEND);
    switch (InBlend) {
        case BlendMode::Alpha:    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
        case BlendMode::Additive: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
        case BlendMode::Multiply: glBlendFunc(GL_DST_COLOR, GL_ZERO); break;
        case BlendMode::Opaque:   break;
    }
}

void ApplyRenderState(const ShaderRenderState& InState) {
    ApplyBlendMode(InState.Blend);

    if (InState.Cull == CullMode::None) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(InState.Cull == CullMode::Back ? GL_BACK : GL_FRONT);
    }

    // Vulkan resolves the winding in a framebuffer whose Y grows downwards, OpenGL in one where it
    // grows upwards, so the same triangle comes out with the opposite winding here.
    glFrontFace(InState.FrontFace == WindingOrder::CounterClockwise ? GL_CW : GL_CCW);

    const bool testsDepth = InState.Depth == DepthMode::Test || InState.Depth == DepthMode::TestWrite;
    const bool writesDepth = InState.Depth == DepthMode::Write || InState.Depth == DepthMode::TestWrite;

    if (testsDepth) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(writesDepth ? GL_TRUE : GL_FALSE);
}

uint32_t GetComponentCount(ShaderDataType InType) {
    switch (InType) {
        case ShaderDataType::Float:
        case ShaderDataType::Int:
        case ShaderDataType::Bool:   return 1;
        case ShaderDataType::Float2:
        case ShaderDataType::Int2:   return 2;
        case ShaderDataType::Float3:
        case ShaderDataType::Int3:   return 3;
        case ShaderDataType::Float4:
        case ShaderDataType::Int4:   return 4;
        case ShaderDataType::Mat3:   return 9;
        case ShaderDataType::Mat4:   return 16;
        case ShaderDataType::None:   break;
    }
    return 0;
}

uint32_t GetTypeSize(ShaderDataType InType) {
    return GetComponentCount(InType) * 4;
}
