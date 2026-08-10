#include "ShaderTranslator.h"

#include <format>
#include <regex>

namespace {

const std::regex s_Version(R"(^\s*#version\b.*$)");
const std::regex s_UniformBlock(R"(^(\s*)layout\s*\(\s*binding\s*=\s*(\d+)\s*(?:,\s*std140\s*)?\)\s*uniform\s+(\w+)\s*\{(.*)$)");
const std::regex s_PushConstantBlock(R"(^(\s*)layout\s*\(\s*push_constant\s*\)\s*uniform\s+(\w+)\s*\{(.*)$)");
const std::regex s_Sampler(R"(^(\s*)layout\s*\(\s*binding\s*=\s*(\d+)\s*\)\s*uniform\s+(\w+)\s+(\w+)\s*;(.*)$)");
const std::regex s_LocatedVarying(R"(^(\s*)layout\s*\(\s*location\s*=\s*\d+\s*\)\s*(in|out)\s+(.*)$)");
const std::regex s_MainSignature(R"(\bvoid\s+main\s*\()");

constexpr const char* s_PrecisionQualifiers =
    "precision highp float; precision highp int; precision highp sampler2D; "
    "precision highp sampler2DArray; precision highp samplerCube; "
    "precision highp sampler2DShadow; precision highp sampler2DArrayShadow;";

constexpr const char* s_TranslatedMain = "ae_StageMain";

// Vulkan clip space keeps depth in [0,w]; OpenGL expects [-w,w].
constexpr const char* s_ClipSpaceFixup =
    "\nvoid main() {\n"
    "    ae_StageMain();\n"
    "    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;\n"
    "}\n";

Array<String> SplitLines(const String& InText) {
    Array<String> lines;
    size_t start = 0;
    while (start <= InText.size()) {
        const size_t end = InText.find('\n', start);
        if (end == String::npos) {
            lines.Add(InText.substr(start));
            break;
        }
        size_t length = end - start;
        if (length > 0 && InText[end - 1] == '\r') {
            length--;
        }
        lines.Add(InText.substr(start, length));
        start = end + 1;
    }
    return lines;
}

void Record(Array<CompiledShaderBinding>& InOutBindings, const String& InName, uint32_t InBinding) {
    for (const CompiledShaderBinding& binding : InOutBindings) {
        if (binding.Name == InName) {
            return;
        }
    }
    InOutBindings.Add({ InName, InBinding });
}

bool IsVaryingDeclaration(ShaderStage InStage, const String& InDirection) {
    return InStage == ShaderStage::Vertex ? InDirection == "out" : InDirection == "in";
}

String TranslateLine(const String& InLine, ShaderStage InStage, TranslatedShader& InOutResult) {
    std::smatch match;

    if (std::regex_match(InLine, match, s_UniformBlock)) {
        Record(InOutResult.UniformBlocks, match[3].str(), (uint32_t)std::stoul(match[2].str()));
        return std::format("{0}layout(std140) uniform {1} {{{2}", match[1].str(), match[3].str(), match[4].str());
    }

    if (std::regex_match(InLine, match, s_PushConstantBlock)) {
        Record(InOutResult.UniformBlocks, match[2].str(), ShaderTranslator::ShaderDataBinding);
        return std::format("{0}layout(std140) uniform {1} {{{2}", match[1].str(), match[2].str(), match[3].str());
    }

    if (std::regex_match(InLine, match, s_Sampler)) {
        Record(InOutResult.Samplers, match[4].str(), (uint32_t)std::stoul(match[2].str()));
        return std::format("{0}uniform {1} {2};{3}", match[1].str(), match[3].str(), match[4].str(), match[5].str());
    }

    if (std::regex_match(InLine, match, s_LocatedVarying) && IsVaryingDeclaration(InStage, match[2].str())) {
        return std::format("{0}{1} {2}", match[1].str(), match[2].str(), match[3].str());
    }

    return InLine;
}

String TranslateStage(const ShaderStageSource& InStage, TranslatedShader& InOutResult) {
    const Array<String> lines = SplitLines(InStage.Source);

    // Line 1 is the #version the preprocessor emitted; #line keeps every following line numbered as
    // it was authored, so compiler diagnostics still point at the original source.
    String translated = std::format("#version 300 es\n{0}\n#line 2\n", s_PrecisionQualifiers);

    const int32_t firstBodyLine = (!lines.IsEmpty() && std::regex_match(lines[0], s_Version)) ? 1 : 0;
    for (int32_t i = firstBodyLine; i < lines.Size(); i++) {
        translated += TranslateLine(lines[i], InStage.Stage, InOutResult) + "\n";
    }

    if (InStage.Stage != ShaderStage::Vertex) {
        return translated;
    }
    return std::regex_replace(translated, s_MainSignature, std::format("void {0}(", s_TranslatedMain)) + s_ClipSpaceFixup;
}

} // namespace

bool ShaderTranslator::Translate(const ShaderSource& InSource, TranslatedShader& OutResult, String& OutError) {
    OutResult = TranslatedShader();

    for (const ShaderStageSource& stage : InSource.GetStages()) {
        if (stage.Stage == ShaderStage::Compute) {
            OutError = "WebGL 2 has no compute shaders";
            return false;
        }
        OutResult.Stages.Add({ stage.Stage, TranslateStage(stage, OutResult) });
    }

    if (OutResult.Stages.IsEmpty()) {
        OutError = "shader declares no stages";
        return false;
    }
    return true;
}
