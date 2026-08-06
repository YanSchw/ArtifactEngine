#pragma once
#include "CoreMinimal.h"
#include "Common/ByteString.h"
#include "ShaderSource.h"
#include "CompiledShader.gen.h"

ARTIFACT_ENUM();
enum class ShaderAPI : uint32_t {
    Unknown,
    Vulkan
};

struct CompiledShaderStage {
    ShaderStage Stage = ShaderStage::Vertex;
    SharedObjectPtr<ByteString> ByteCode;
};

/** A shader after its stages have been translated to a backend's bytecode. This is what the cooked
 *  shader library stores and what a RenderingAPI turns into a native shader. */
class CompiledShader {
public:
    ShaderAPI API = ShaderAPI::Unknown;
    ShaderRenderState RenderState;
    Array<CompiledShaderStage> Stages;

    const CompiledShaderStage* FindStage(ShaderStage InStage) const;
    bool HasStage(ShaderStage InStage) const { return FindStage(InStage) != nullptr; }
    bool IsValid() const;

    void Serialize(class ChunkWriter& OutWriter) const;
    bool Deserialize(class ChunkReader& InReader);
};
