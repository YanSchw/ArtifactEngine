#include "ShaderSource.h"

#include "Core/EngineConfig.h"
#include "Object/Enum.h"
#include "Platform/FileIO.h"

#include <cctype>
#include <filesystem>
#include <format>
#include <functional>

namespace {

struct FlatLine {
    String Text;
    String File;
    uint32_t Line = 0;
};

struct Directive {
    String Name;
    String Arguments;
};

Array<String> SplitLines(const String& InText) {
    Array<String> lines;
    size_t start = 0;
    while (start <= InText.size()) {
        size_t end = InText.find('\n', start);
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

String Trim(const String& InText) {
    size_t begin = InText.find_first_not_of(" \t");
    if (begin == String::npos) {
        return "";
    }
    size_t end = InText.find_last_not_of(" \t");
    return InText.substr(begin, end - begin + 1);
}

bool ParseDirective(const String& InLine, Directive& OutDirective) {
    const String trimmed = Trim(InLine);
    if (trimmed.size() < 2 || trimmed[0] != '#') {
        return false;
    }

    size_t nameBegin = trimmed.find_first_not_of(" \t", 1);
    if (nameBegin == String::npos) {
        return false;
    }

    size_t nameEnd = nameBegin;
    while (nameEnd < trimmed.size() && (std::isalnum((unsigned char)trimmed[nameEnd]) || trimmed[nameEnd] == '_')) {
        nameEnd++;
    }
    if (nameEnd == nameBegin) {
        return false;
    }

    OutDirective.Name = trimmed.substr(nameBegin, nameEnd - nameBegin);
    OutDirective.Arguments = Trim(trimmed.substr(nameEnd));
    return true;
}

bool ParseIncludePath(const String& InArguments, String& OutPath) {
    if (InArguments.size() < 2) {
        return false;
    }
    const char opening = InArguments[0];
    const char closing = opening == '<' ? '>' : opening;
    if (opening != '"' && opening != '<') {
        return false;
    }
    const size_t end = InArguments.find(closing, 1);
    if (end == String::npos || end == 1) {
        return false;
    }
    OutPath = InArguments.substr(1, end - 1);
    return true;
}

String NormalizePath(const String& InPath) {
    std::error_code error;
    std::filesystem::path canonical = std::filesystem::weakly_canonical(std::filesystem::path(InPath), error);
    if (error) {
        return InPath;
    }
    return canonical.string();
}

String ResolveIncludePath(const String& InIncludePath, const String& InIncludingFile) {
    if (!InIncludePath.empty() && (InIncludePath[0] == '/' || InIncludePath[0] == '\\')) {
        return NormalizePath(EngineConfig::ResolveContentPath(InIncludePath));
    }

    const std::filesystem::path relative = std::filesystem::path(InIncludingFile).parent_path() / InIncludePath;
    if (FileIO::FileExists(relative.string())) {
        return NormalizePath(relative.string());
    }

    return NormalizePath(EngineConfig::ResolveContentPath("/" + InIncludePath));
}

template<typename T>
bool ParseEnumArgument(const String& InEnumName, const String& InArgument, T& OutValue) {
    for (const Enum::EnumValue& value : Enum(InEnumName).GetValues()) {
        if (value.Name == InArgument) {
            OutValue = (T)value.Value;
            return true;
        }
    }
    return false;
}

String DescribeEnumValues(const String& InEnumName) {
    String result;
    for (const Enum::EnumValue& value : Enum(InEnumName).GetValues()) {
        if (!result.empty()) {
            result += ", ";
        }
        result += value.Name;
    }
    return result;
}

} // namespace

ShaderSourceLocation ShaderStageSource::ResolveLine(uint32_t InGeneratedLine) const {
    const int32_t index = (int32_t)InGeneratedLine - 1;
    if (index < 0 || index >= LineMap.Size()) {
        return ShaderSourceLocation{};
    }
    return LineMap[index];
}

String ShaderSource::GetStageName(ShaderStage InStage) {
    switch (InStage) {
        case ShaderStage::Vertex:   return "vert";
        case ShaderStage::Fragment: return "frag";
        case ShaderStage::Compute:  return "comp";
    }
    return "";
}

bool ShaderSource::ParseStageName(const String& InName, ShaderStage& OutStage) {
    if (InName == "vert" || InName == "vertex" || InName == "Vertex") {
        OutStage = ShaderStage::Vertex;
        return true;
    }
    if (InName == "frag" || InName == "fragment" || InName == "Fragment") {
        OutStage = ShaderStage::Fragment;
        return true;
    }
    if (InName == "comp" || InName == "compute" || InName == "Compute") {
        OutStage = ShaderStage::Compute;
        return true;
    }
    return false;
}

bool ShaderSource::DeclaresStage(const String& InSource) {
    for (const String& line : SplitLines(InSource)) {
        Directive directive;
        if (ParseDirective(line, directive) && (directive.Name == "type" || directive.Name == "stage")) {
            return true;
        }
    }
    return false;
}

const ShaderStageSource* ShaderSource::FindStage(ShaderStage InStage) const {
    for (const ShaderStageSource& stage : m_Stages) {
        if (stage.Stage == InStage) {
            return &stage;
        }
    }
    return nullptr;
}

bool ShaderSource::Preprocess(const String& InPath, ShaderSource& OutSource, String& OutError) {
    const String resolved = NormalizePath(EngineConfig::ResolveContentPath(InPath));
    if (!FileIO::FileExists(resolved)) {
        OutError = std::format("shader '{0}' not found", InPath);
        return false;
    }
    return PreprocessSource(resolved, FileIO::ReadFileToString(resolved), OutSource, OutError);
}

bool ShaderSource::PreprocessSource(const String& InPath, const String& InSource, ShaderSource& OutSource, String& OutError) {
    OutSource = ShaderSource();
    OutSource.m_Path = InPath;

    Array<FlatLine> flattened;
    Array<String> includeStack;
    Array<String> includedOnce;
    String versionDirective;

    std::function<bool(const String&, const String&)> flatten =
        [&](const String& InFilePath, const String& InFileSource) -> bool {
        if (includeStack.Contains(InFilePath)) {
            OutError = std::format("circular #include of '{0}'", InFilePath);
            return false;
        }
        includeStack.Add(InFilePath);

        const Array<String> lines = SplitLines(InFileSource);
        for (int32_t i = 0; i < lines.Size(); i++) {
            const String& line = lines[i];
            const uint32_t lineNumber = (uint32_t)i + 1;

            Directive directive;
            if (ParseDirective(line, directive)) {
                if (directive.Name == "include") {
                    String includePath;
                    if (!ParseIncludePath(directive.Arguments, includePath)) {
                        OutError = std::format("{0}:{1}: malformed #include", InFilePath, lineNumber);
                        return false;
                    }

                    const String resolved = ResolveIncludePath(includePath, InFilePath);
                    if (!FileIO::FileExists(resolved)) {
                        OutError = std::format("{0}:{1}: cannot find included file '{2}'", InFilePath, lineNumber, includePath);
                        return false;
                    }

                    if (includedOnce.Contains(resolved)) {
                        continue;
                    }
                    if (!OutSource.m_Dependencies.Contains(resolved)) {
                        OutSource.m_Dependencies.Add(resolved);
                    }
                    if (!flatten(resolved, FileIO::ReadFileToString(resolved))) {
                        return false;
                    }
                    continue;
                }

                if (directive.Name == "pragma" && Trim(directive.Arguments) == "once") {
                    if (!includedOnce.Contains(InFilePath)) {
                        includedOnce.Add(InFilePath);
                    }
                    continue;
                }

                if (directive.Name == "shadergraph") {
                    OutError = std::format("{0}:{1}: this shader is a shader-graph template and cannot be compiled on its own",
                                           InFilePath, lineNumber);
                    return false;
                }

                if (directive.Name == "version") {
                    if (versionDirective.empty()) {
                        versionDirective = "#version " + directive.Arguments;
                    }
                    continue;
                }
            }

            flattened.Add(FlatLine{ line, InFilePath, lineNumber });
        }

        includeStack.RemoveAt(includeStack.Last());
        return true;
    };

    if (!flatten(InPath, InSource)) {
        return false;
    }

    if (versionDirective.empty()) {
        versionDirective = "#version 450";
    }

    Array<FlatLine> prelude;
    Array<FlatLine> current;
    bool hasStage = false;
    ShaderStage currentStage = ShaderStage::Vertex;

    auto emitStage = [&]() -> bool {
        if (!hasStage) {
            return true;
        }
        if (OutSource.FindStage(currentStage)) {
            OutError = std::format("{0}: duplicate #type {1}", InPath, GetStageName(currentStage));
            return false;
        }

        ShaderStageSource stage;
        stage.Stage = currentStage;
        stage.Source = versionDirective + "\n";
        stage.LineMap.Add(ShaderSourceLocation{ InPath, 1 });

        for (const FlatLine& line : prelude) {
            stage.Source += line.Text + "\n";
            stage.LineMap.Add(ShaderSourceLocation{ line.File, line.Line });
        }
        for (const FlatLine& line : current) {
            stage.Source += line.Text + "\n";
            stage.LineMap.Add(ShaderSourceLocation{ line.File, line.Line });
        }

        OutSource.m_Stages.Add(stage);
        current.Clear();
        return true;
    };

    for (const FlatLine& line : flattened) {
        Directive directive;
        if (ParseDirective(line.Text, directive)) {
            if (directive.Name == "type" || directive.Name == "stage") {
                ShaderStage stage;
                if (!ParseStageName(Trim(directive.Arguments), stage)) {
                    OutError = std::format("{0}:{1}: unknown shader stage '{2}'", line.File, line.Line, directive.Arguments);
                    return false;
                }
                if (!emitStage()) {
                    return false;
                }
                currentStage = stage;
                hasStage = true;
                continue;
            }

            if (directive.Name == "blend" || directive.Name == "cull" ||
                directive.Name == "depth" || directive.Name == "frontface") {
                if (hasStage) {
                    OutError = std::format("{0}:{1}: #{2} must appear before the first #type",
                                           line.File, line.Line, directive.Name);
                    return false;
                }

                const String argument = Trim(directive.Arguments);
                if (argument.starts_with("(")) {
                    OutError = std::format("{0}:{1}: #{2} option lists are only valid in a shader-graph template",
                                           line.File, line.Line, directive.Name);
                    return false;
                }

                bool parsed = false;
                String enumName;
                if (directive.Name == "blend") {
                    enumName = "BlendMode";
                    parsed = ParseEnumArgument(enumName, argument, OutSource.m_RenderState.Blend);
                } else if (directive.Name == "cull") {
                    enumName = "CullMode";
                    parsed = ParseEnumArgument(enumName, argument, OutSource.m_RenderState.Cull);
                } else if (directive.Name == "depth") {
                    enumName = "DepthMode";
                    parsed = ParseEnumArgument(enumName, argument, OutSource.m_RenderState.Depth);
                } else {
                    enumName = "WindingOrder";
                    parsed = ParseEnumArgument(enumName, argument, OutSource.m_RenderState.FrontFace);
                }

                if (!parsed) {
                    OutError = std::format("{0}:{1}: unknown #{2} value '{3}' (expected one of: {4})",
                                           line.File, line.Line, directive.Name, argument, DescribeEnumValues(enumName));
                    return false;
                }
                continue;
            }
        }

        if (hasStage) {
            current.Add(line);
        } else {
            prelude.Add(line);
        }
    }

    if (!emitStage()) {
        return false;
    }

    if (OutSource.m_Stages.IsEmpty()) {
        OutError = std::format("{0}: shader declares no #type stage", InPath);
        return false;
    }

    const bool hasVertex = OutSource.HasStage(ShaderStage::Vertex);
    const bool hasFragment = OutSource.HasStage(ShaderStage::Fragment);
    const bool hasCompute = OutSource.HasStage(ShaderStage::Compute);

    if (hasCompute && (hasVertex || hasFragment)) {
        OutError = std::format("{0}: compute stage cannot be combined with graphics stages", InPath);
        return false;
    }
    if (!hasCompute && hasVertex != hasFragment) {
        OutError = std::format("{0}: a graphics shader needs both a vertex and a fragment stage", InPath);
        return false;
    }

    return true;
}
