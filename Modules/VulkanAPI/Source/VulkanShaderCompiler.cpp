#include "VulkanShaderCompiler.h"

#include "Core/EngineConfig.h"
#include "Platform/FileIO.h"
#include "Platform/Platform.h"
#include "Platform/Subprocess.h"

#include <cctype>
#include <format>

static String GetGlslCompilerPath() {
    switch (Platform::CurrentPlatform()) {
        case PlatformType::Win64: return "glslangValidator.exe";
        case PlatformType::MacOS: return "/usr/local/bin/glslangValidator";
        case PlatformType::Linux: return "/usr/bin/glslangValidator";
        default: return "";
    }
}

static String GetStageFileExtension(ShaderStage InStage) {
    switch (InStage) {
        case ShaderStage::Vertex:   return "vert";
        case ShaderStage::Fragment: return "frag";
        case ShaderStage::Compute:  return "comp";
    }
    return "";
}

static String RemapDiagnostics(const String& InOutput, const String& InTempPath, const ShaderStageSource& InStage, const String& InShaderPath) {
    String remapped;
    size_t lineStart = 0;

    while (lineStart <= InOutput.size()) {
        size_t lineEnd = InOutput.find('\n', lineStart);
        const bool isLastLine = lineEnd == String::npos;
        String line = InOutput.substr(lineStart, isLastLine ? String::npos : lineEnd - lineStart);

        size_t match = line.find(InTempPath);
        while (match != String::npos) {
            size_t cursor = match + InTempPath.size();
            if (cursor < line.size() && line[cursor] == ':') {
                size_t digitsBegin = cursor + 1;
                size_t digitsEnd = digitsBegin;
                while (digitsEnd < line.size() && std::isdigit((unsigned char)line[digitsEnd])) {
                    digitsEnd++;
                }

                if (digitsEnd > digitsBegin) {
                    const uint32_t generatedLine = (uint32_t)std::stoul(line.substr(digitsBegin, digitsEnd - digitsBegin));
                    const ShaderSourceLocation location = InStage.ResolveLine(generatedLine);
                    const String replacement = location.File.empty()
                        ? std::format("{0}:{1}", InShaderPath, generatedLine)
                        : std::format("{0}:{1}", location.File, location.Line);

                    line = line.substr(0, match) + replacement + line.substr(digitsEnd);
                    match = line.find(InTempPath, match + replacement.size());
                    continue;
                }
            }

            line = line.substr(0, match) + InShaderPath + line.substr(match + InTempPath.size());
            match = line.find(InTempPath, match + InShaderPath.size());
        }

        remapped += line;
        if (isLastLine) {
            break;
        }
        remapped += "\n";
        lineStart = lineEnd + 1;
    }

    return remapped;
}

bool VulkanShaderCompiler::SupportsPlatform(PlatformType InPlatform) const {
    return InPlatform == PlatformType::Win64
        || InPlatform == PlatformType::MacOS
        || InPlatform == PlatformType::Linux;
}

bool VulkanShaderCompiler::Compile(const ShaderSource& InSource, CompiledShader& OutCompiled, String& OutError) {
    const String compilerPath = GetGlslCompilerPath();
    if (compilerPath.empty()) {
        OutError = "no GLSL compiler available on this platform";
        return false;
    }

    OutCompiled = CompiledShader();
    OutCompiled.API = ShaderAPI::Vulkan;
    OutCompiled.RenderState = InSource.GetRenderState();

    Platform::TemporaryDirectory tempDir;

    for (const ShaderStageSource& stage : InSource.GetStages()) {
        const String extension = GetStageFileExtension(stage.Stage);
        const String sourcePath = std::format("{0}/shader.{1}", tempDir.Path, extension);
        const String outputPath = std::format("{0}/shader.{1}.spv", tempDir.Path, extension);

        if (!FileIO::WriteStringToFile(sourcePath, stage.Source)) {
            OutError = std::format("failed to write temporary shader source to '{0}'", sourcePath);
            return false;
        }

        const SubprocessResult result = Subprocess::Run(
            std::format("\"{0}\" -V -S {1} \"{2}\" -o \"{3}\"", compilerPath, extension, sourcePath, outputPath));

        if (result.ExitCode != 0) {
            const String diagnostics = result.StdOut.empty() ? result.StdErr : result.StdOut;
            OutError = RemapDiagnostics(diagnostics, sourcePath, stage, InSource.GetPath());
            return false;
        }

        SharedObjectPtr<ByteString> byteCode = FileIO::ReadFileToBytes(outputPath);
        if (!byteCode || byteCode->GetSizeInBytes() == 0) {
            OutError = std::format("compiler produced no SPIR-V for the {0} stage", extension);
            return false;
        }

        CompiledShaderStage compiledStage;
        compiledStage.Stage = stage.Stage;
        compiledStage.ByteCode = byteCode;
        OutCompiled.Stages.Add(compiledStage);
    }

    return OutCompiled.IsValid();
}
