#pragma once
#include "CoreMinimal.h"
#include "Rendering/CompiledShader.h"
#include "Rendering/ShaderSource.h"

struct TranslatedStage {
    ShaderStage Stage = ShaderStage::Vertex;
    String Source;
};

struct TranslatedShader {
    Array<TranslatedStage> Stages;
    Array<CompiledShaderBinding> UniformBlocks;
    Array<CompiledShaderBinding> Samplers;
};

/** Rewrites the engine's Vulkan-flavoured GLSL 4.50 into the GLSL ES 3.00 that WebGL 2 accepts.
 *
 *  ES 3.00 has no `layout(binding = ...)` and no push constants, so resource slots move out of the
 *  source into a side table the backend replays through glUniformBlockBinding / glUniform1i, and
 *  varying locations are dropped (ES 3.00 matches varyings by name). Clip space is converted from
 *  Vulkan's [0,w] depth to OpenGL's [-w,w] in a wrapper around the vertex stage's main(). */
class ShaderTranslator {
public:
    /** Uniform block slot the push-constant block is rebound to. */
    static constexpr uint32_t ShaderDataBinding = 15;

    static bool Translate(const ShaderSource& InSource, TranslatedShader& OutResult, String& OutError);
};
