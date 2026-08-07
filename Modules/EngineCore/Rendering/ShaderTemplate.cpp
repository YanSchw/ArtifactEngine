#include "ShaderTemplate.h"

#include "Core/EngineConfig.h"
#include "Platform/FileIO.h"

#include <cctype>
#include <format>

namespace {

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

String Unquote(const String& InText) {
    if (InText.size() >= 2 && InText.front() == '"' && InText.back() == '"') {
        return InText.substr(1, InText.size() - 2);
    }
    return InText;
}

Array<String> SplitArguments(const String& InText) {
    Array<String> arguments;
    int32_t depth = 0;
    size_t start = 0;
    for (size_t i = 0; i < InText.size(); i++) {
        const char c = InText[i];
        if (c == '(' || c == '[') {
            depth++;
        } else if (c == ')' || c == ']') {
            depth--;
        } else if (c == ',' && depth == 0) {
            arguments.Add(Trim(InText.substr(start, i - start)));
            start = i + 1;
        }
    }
    arguments.Add(Trim(InText.substr(start)));
    return arguments;
}

bool ParseCall(const String& InLine, const String& InName, String& OutArguments, size_t& OutBegin, size_t& OutEnd) {
    const String token = "#" + InName;
    const size_t begin = InLine.find(token);
    if (begin == String::npos) {
        return false;
    }

    size_t cursor = InLine.find_first_not_of(" \t", begin + token.size());
    if (cursor == String::npos || InLine[cursor] != '(') {
        return false;
    }

    int32_t depth = 0;
    for (size_t i = cursor; i < InLine.size(); i++) {
        if (InLine[i] == '(') {
            depth++;
        } else if (InLine[i] == ')') {
            depth--;
            if (depth == 0) {
                OutArguments = InLine.substr(cursor + 1, i - cursor - 1);
                OutBegin = begin;
                OutEnd = i + 1;
                return true;
            }
        }
    }
    return false;
}

Vec4 ParseDefaultValue(const String& InText, ShaderValueType InType) {
    Vec4 value(0.0f);
    const size_t open = InText.find('(');
    if (open == String::npos) {
        try {
            value = Vec4(std::stof(Trim(InText)));
        } catch (const std::exception&) {
            value = Vec4(0.0f);
        }
        if (InType == ShaderValueType::Float) {
            return value;
        }
        return value;
    }

    const size_t close = InText.rfind(')');
    const Array<String> components = SplitArguments(InText.substr(open + 1, close - open - 1));
    for (int32_t i = 0; i < components.Size() && i < 4; i++) {
        try {
            value[i] = std::stof(components[i]);
        } catch (const std::exception&) {
            value[i] = 0.0f;
        }
    }
    if (components.Size() == 1) {
        value = Vec4(value.x);
    }
    return value;
}

const char* s_SceneBlock =
    "layout(binding = 0, std140) uniform SceneBlock {\n"
    "    mat4 u_ViewProjection;\n"
    "    float u_Time;\n"
    "};\n"
    "layout(push_constant) uniform ShaderDataBlock {\n"
    "    mat4 WorldTransform;\n"
    "    uint NodeId;\n"
    "} u_ShaderData;\n";

} // namespace

String ShaderTemplate::GetStageDeclarations(ShaderStage InStage) {
    if (InStage == ShaderStage::Vertex) {
        return String(
            "layout(location = 0) in vec3 a_Position;\n"
            "layout(location = 1) in vec3 a_Color;\n"
            "layout(location = 2) in vec2 a_UV;\n"
            "layout(location = 1) out vec4 v_Color;\n"
            "layout(location = 2) out vec2 v_UV;\n"
            "layout(location = 3) out vec3 v_WorldPosition;\n") + s_SceneBlock;
    }

    return String(
        "layout(location = 1) in vec4 v_Color;\n"
        "layout(location = 2) in vec2 v_UV;\n"
        "layout(location = 3) in vec3 v_WorldPosition;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "layout(location = 1) out vec4 outNodeId;\n") + s_SceneBlock;
}

String ShaderTemplate::GetStageAliases(ShaderStage InStage) {
    if (InStage == ShaderStage::Vertex) {
        return "#define sg_UV a_UV\n"
               "#define sg_VertexColor vec4(a_Color, 1.0)\n"
               "#define sg_WorldPosition (u_ShaderData.WorldTransform * vec4(a_Position, 1.0)).xyz\n"
               "#define sg_Time u_Time\n";
    }

    return "#define sg_UV v_UV\n"
           "#define sg_VertexColor v_Color\n"
           "#define sg_WorldPosition v_WorldPosition\n"
           "#define sg_Time u_Time\n";
}

bool ShaderTemplate::IsTemplateSource(const String& InSource) {
    for (const String& line : SplitLines(InSource)) {
        if (Trim(line).starts_with(Directive)) {
            return true;
        }
    }
    return false;
}

Array<ShaderTemplateInfo> ShaderTemplate::FindAll() {
    Array<ShaderTemplateInfo> templates;

    for (const String& mountDir : EngineConfig::GetContentMountDirs()) {
        const std::filesystem::path mountPath(mountDir);
        for (const String& file : FileIO::ListFilesInDirectory(mountDir, true)) {
            const std::filesystem::path filePath(file);
            if (filePath.extension().string() != ".glsl") {
                continue;
            }

            const String source = FileIO::ReadFileToString(file);
            if (!IsTemplateSource(source)) {
                continue;
            }

            std::error_code error;
            const std::filesystem::path relative = std::filesystem::relative(filePath, mountPath, error);
            if (error) {
                continue;
            }

            ShaderTemplate parsed;
            String parseError;
            ShaderTemplateInfo info;
            info.Path = "/" + relative.generic_string();
            info.DisplayName = ParseSource(info.Path, source, parsed, parseError)
                ? parsed.GetDisplayName()
                : info.Path;

            bool duplicate = false;
            for (const ShaderTemplateInfo& existing : templates) {
                duplicate = duplicate || existing.Path == info.Path;
            }
            if (!duplicate) {
                templates.Add(info);
            }
        }
    }

    return templates;
}

bool ShaderTemplate::Parse(const String& InPath, ShaderTemplate& OutTemplate, String& OutError) {
    const String resolved = EngineConfig::ResolveContentPath(InPath);
    if (!FileIO::FileExists(resolved)) {
        OutError = std::format("shader template '{0}' not found", InPath);
        return false;
    }
    return ParseSource(InPath, FileIO::ReadFileToString(resolved), OutTemplate, OutError);
}

bool ShaderTemplate::ParseSource(const String& InPath, const String& InSource, ShaderTemplate& OutTemplate, String& OutError) {
    OutTemplate = ShaderTemplate();
    OutTemplate.m_Path = InPath;
    OutTemplate.m_Lines = SplitLines(InSource);

    ShaderStage stage = ShaderStage::Vertex;

    for (int32_t i = 0; i < OutTemplate.m_Lines.Size(); i++) {
        const String& line = OutTemplate.m_Lines[i];
        const String trimmed = Trim(line);

        if (trimmed.starts_with(Directive)) {
            OutTemplate.m_DisplayName = Unquote(Trim(trimmed.substr(strlen(Directive))));
            continue;
        }

        for (const String& state : { String("blend"), String("cull"), String("depth"), String("frontface") }) {
            const String token = "#" + state;
            if (!trimmed.starts_with(token)) {
                continue;
            }
            const char delimiter = trimmed.size() > token.size() ? trimmed[token.size()] : ' ';
            if (delimiter != ' ' && delimiter != '\t' && delimiter != '(') {
                continue;
            }
            const String argument = Trim(trimmed.substr(token.size()));
            Array<String> values;
            if (argument.starts_with("(")) {
                const size_t close = argument.find(')');
                if (close == String::npos) {
                    OutError = std::format("{0}:{1}: unterminated #{2} option list", InPath, i + 1, state);
                    return false;
                }
                for (const String& value : SplitArguments(argument.substr(1, close - 1))) {
                    if (!value.empty()) {
                        values.Add(value);
                    }
                }
            }
            if (!values.IsEmpty()) {
                OutTemplate.m_StateOptions[state] = values;
                if (!OutTemplate.m_StateOrder.Contains(state)) {
                    OutTemplate.m_StateOrder.Add(state);
                }
            }
        }

        if (trimmed.starts_with("#type") || trimmed.starts_with("#stage")) {
            const String name = Trim(trimmed.substr(trimmed.find_first_of(" \t")));
            if (!ShaderSource::ParseStageName(name, stage)) {
                OutError = std::format("{0}:{1}: unknown shader stage '{2}'", InPath, i + 1, name);
                return false;
            }
            continue;
        }

        String arguments;
        size_t begin = 0;
        size_t end = 0;

        if (trimmed.starts_with("#gen_buffers") && !ParseCall(line, "gen_buffers", arguments, begin, end)) {
            OutError = std::format("{0}:{1}: #gen_buffers expects a stage, e.g. #gen_buffers(frag)", InPath, i + 1);
            return false;
        }

        if (!ParseCall(line, "property", arguments, begin, end)) {
            continue;
        }

        const Array<String> parts = SplitArguments(arguments);
        if (parts.Size() != 4) {
            OutError = std::format("{0}:{1}: #property expects (identifier, \"Name\", type, default)", InPath, i + 1);
            return false;
        }

        ShaderPropertySite site;
        site.Identifier = parts[0];
        site.PropertyName = Unquote(parts[1]);
        site.Stage = stage;
        site.LineIndex = (uint32_t)i;

        if (!ShaderValue::ParseGlslType(parts[2], site.Type)) {
            OutError = std::format("{0}:{1}: unknown #property type '{2}'", InPath, i + 1, parts[2]);
            return false;
        }
        if (site.Type == ShaderValueType::Texture2D) {
            OutError = std::format("{0}:{1}: textures come from graph nodes, not #property declarations",
                                   InPath, i + 1);
            return false;
        }

        if (const ShaderGraphProperty* existing = OutTemplate.FindProperty(site.PropertyName)) {
            if (existing->Type != site.Type) {
                OutError = std::format("{0}:{1}: property '{2}' redeclared with a different type",
                                       InPath, i + 1, site.PropertyName);
                return false;
            }
        } else {
            ShaderGraphProperty property;
            property.Name = site.PropertyName;
            property.Identifier = site.Identifier;
            property.Type = site.Type;
            property.DefaultValue = ParseDefaultValue(parts[3], site.Type);
            OutTemplate.m_Properties.Add(property);
        }

        OutTemplate.m_Sites.Add(site);
    }

    if (OutTemplate.m_DisplayName.empty()) {
        OutTemplate.m_DisplayName = std::filesystem::path(InPath).stem().string();
    }

    return true;
}

Array<String> ShaderTemplate::GetStateOptions(const String& InState) const {
    return m_StateOptions.ContainsKey(InState) ? m_StateOptions.At(InState) : Array<String>();
}

Array<String> ShaderTemplate::GetStateNames() const {
    return m_StateOrder;
}

String ShaderTemplate::GetDefaultStateValue(const String& InState) const {
    const Array<String> options = GetStateOptions(InState);
    return options.IsEmpty() ? String() : options[0];
}

const ShaderGraphProperty* ShaderTemplate::FindProperty(const String& InName) const {
    for (const ShaderGraphProperty& property : m_Properties) {
        if (property.Name == InName) {
            return &property;
        }
    }
    return nullptr;
}

String ShaderTemplate::Expand(const Map<String, ShaderPropertyCode>& InCode, const Map<String, String>& InStates,
                              const String& InDeclarations) const {
    String result;
    for (int32_t i = 0; i < m_Lines.Size(); i++) {
        const String& line = m_Lines[i];
        const String trimmed = Trim(line);

        if (trimmed.starts_with(Directive)) {
            continue;
        }

        bool wroteState = false;
        for (const String& state : m_StateOrder) {
            if (!trimmed.starts_with("#" + state)) {
                continue;
            }
            String value = InStates.ContainsKey(state) ? InStates.At(state) : String();
            const Array<String> options = GetStateOptions(state);
            if (!options.Contains(value)) {
                value = options.IsEmpty() ? String() : options[0];
            }
            if (!value.empty()) {
                result += "#" + state + " " + value + "\n";
            }
            wroteState = true;
            break;
        }
        if (wroteState) {
            continue;
        }

        String arguments;
        size_t begin = 0;
        size_t end = 0;

        if (ParseCall(line, "gen_buffers", arguments, begin, end)) {
            ShaderStage bufferStage = ShaderStage::Fragment;
            ShaderSource::ParseStageName(Trim(arguments), bufferStage);

            result += GetStageDeclarations(bufferStage);
            result += InDeclarations;
            result += GetStageAliases(bufferStage);
            continue;
        }

        if (ParseCall(line, "property", arguments, begin, end)) {
            const ShaderPropertySite* site = nullptr;
            for (const ShaderPropertySite& candidate : m_Sites) {
                if (candidate.LineIndex == (uint32_t)i) {
                    site = &candidate;
                    break;
                }
            }
            if (!site) {
                continue;
            }

            String expression = "Material." + site->PropertyName;
            if (InCode.ContainsKey(site->PropertyName)) {
                const ShaderPropertyCode& code = InCode.At(site->PropertyName);
                result += code.Prelude;
                expression = code.Expression;
            }

            const String declaration = std::format("{0} {1} = {2};",
                                                   ShaderValue::GetGlslType(site->Type), site->Identifier, expression);
            result += line.substr(0, begin) + declaration + line.substr(end) + "\n";
            continue;
        }

        result += line + "\n";
    }

    return result;
}
