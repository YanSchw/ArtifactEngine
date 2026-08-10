#pragma once
#include "CoreMinimal.h"
#include "Common/ByteString.h"
#include "ShaderSource.h"
#include "CompiledShader.gen.h"

ARTIFACT_ENUM();
enum class ShaderAPI : uint32_t {
    Unknown,
    Vulkan,
    WebGL
};

struct CompiledShaderStage {
    ShaderStage Stage = ShaderStage::Vertex;
    SharedObjectPtr<ByteString> ByteCode;
};

/** One named resource and the binding slot it was declared with. Backends whose shading language
 *  cannot carry an explicit binding (GLSL ES) resolve the name against this at link time. */
struct CompiledShaderBinding {
    String Name;
    uint32_t Binding = 0;
};

/** A shader after its stages have been translated to a backend's bytecode. This is what the cooked
 *  shader library stores and what a RenderingAPI turns into a native shader. */
class CompiledShader {
public:
    ShaderAPI API = ShaderAPI::Unknown;
    ShaderRenderState RenderState;
    Array<CompiledShaderStage> Stages;
    Array<CompiledShaderBinding> UniformBlocks;
    Array<CompiledShaderBinding> Samplers;

    const CompiledShaderStage* FindStage(ShaderStage InStage) const;
    bool HasStage(ShaderStage InStage) const { return FindStage(InStage) != nullptr; }
    bool IsValid() const;

    void Serialize(class ChunkWriter& OutWriter) const;
    bool Deserialize(class ChunkReader& InReader);

private:
    static void SerializeBindings(class ChunkWriter& OutWriter, const Array<CompiledShaderBinding>& InBindings);
    static void DeserializeBindings(class ChunkReader& InReader, Array<CompiledShaderBinding>& OutBindings);
};
