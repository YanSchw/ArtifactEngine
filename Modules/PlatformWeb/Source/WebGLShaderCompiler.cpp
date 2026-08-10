#include "WebGLShaderCompiler.h"

#include "ShaderTranslator.h"

#include <cstring>

bool WebGLShaderCompiler::Compile(const ShaderSource& InSource, CompiledShader& OutCompiled, String& OutError) {
    TranslatedShader translated;
    if (!ShaderTranslator::Translate(InSource, translated, OutError)) {
        return false;
    }

    OutCompiled = CompiledShader();
    OutCompiled.API = ShaderAPI::WebGL;
    OutCompiled.RenderState = InSource.GetRenderState();
    OutCompiled.UniformBlocks = translated.UniformBlocks;
    OutCompiled.Samplers = translated.Samplers;

    for (const TranslatedStage& stage : translated.Stages) {
        byte* source = new byte[stage.Source.size()];
        memcpy(source, stage.Source.data(), stage.Source.size());

        CompiledShaderStage compiledStage;
        compiledStage.Stage = stage.Stage;
        compiledStage.ByteCode = new ByteString(stage.Source.size(), source);
        OutCompiled.Stages.Add(compiledStage);
    }

    return OutCompiled.IsValid();
}
