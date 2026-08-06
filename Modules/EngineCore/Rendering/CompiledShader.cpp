#include "CompiledShader.h"

#include "Serialization/ChunkedBinary.h"

const CompiledShaderStage* CompiledShader::FindStage(ShaderStage InStage) const {
    for (const CompiledShaderStage& stage : Stages) {
        if (stage.Stage == InStage) {
            return &stage;
        }
    }
    return nullptr;
}

bool CompiledShader::IsValid() const {
    if (API == ShaderAPI::Unknown || Stages.IsEmpty()) {
        return false;
    }
    for (const CompiledShaderStage& stage : Stages) {
        if (!stage.ByteCode || stage.ByteCode->GetSizeInBytes() == 0) {
            return false;
        }
    }
    return true;
}

void CompiledShader::Serialize(ChunkWriter& OutWriter) const {
    OutWriter << (uint32_t)API;
    OutWriter << (uint32_t)RenderState.Blend;
    OutWriter << (uint32_t)RenderState.Cull;
    OutWriter << (uint32_t)RenderState.Depth;
    OutWriter << (uint32_t)RenderState.FrontFace;

    OutWriter << (uint32_t)Stages.Size();
    for (const CompiledShaderStage& stage : Stages) {
        OutWriter << (uint32_t)stage.Stage;
        OutWriter << (uint64_t)stage.ByteCode->GetSizeInBytes();
        OutWriter.WriteBytes(stage.ByteCode->GetData(), stage.ByteCode->GetSizeInBytes());
    }
}

bool CompiledShader::Deserialize(ChunkReader& InReader) {
    uint32_t api = 0;
    uint32_t blend = 0;
    uint32_t cull = 0;
    uint32_t depth = 0;
    uint32_t frontFace = 0;
    uint32_t stageCount = 0;

    InReader >> api;
    InReader >> blend;
    InReader >> cull;
    InReader >> depth;
    InReader >> frontFace;
    InReader >> stageCount;

    API = (ShaderAPI)api;
    RenderState.Blend = (BlendMode)blend;
    RenderState.Cull = (CullMode)cull;
    RenderState.Depth = (DepthMode)depth;
    RenderState.FrontFace = (WindingOrder)frontFace;

    Stages.Clear();
    for (uint32_t i = 0; i < stageCount; i++) {
        uint32_t stage = 0;
        uint64_t size = 0;
        InReader >> stage;
        InReader >> size;

        if (size > InReader.GetRemaining()) {
            return false;
        }

        byte* data = new byte[size];
        InReader.ReadBytes(data, size);

        CompiledShaderStage compiledStage;
        compiledStage.Stage = (ShaderStage)stage;
        compiledStage.ByteCode = new ByteString((size_t)size, data);
        Stages.Add(compiledStage);
    }

    return IsValid();
}
