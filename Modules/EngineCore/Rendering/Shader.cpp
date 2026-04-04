#include "Shader.h"
#include "RenderingAPI.h"

SharedObjectPtr<Shader> Shader::Create(const String& InShaderSource) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateShader(InShaderSource);
}

Map<String, String> Shader::PreProcess(const String& InShaderSource) {
    Map<String, String> shaderSources;

    const char* typeToken = "#type";
    size_t typeTokenLength = strlen(typeToken);
    size_t pos = InShaderSource.find(typeToken, 0); // Start of shader type declaration line
    while (pos != String::npos) {
        size_t eol = InShaderSource.find_first_of("\r\n", pos); // End of shader type declaration line
        AE_ASSERT(eol != String::npos, "Syntax error");
        size_t begin = pos + typeTokenLength + 1; // Start of shader type name (after "#type " keyword)
        String type = InShaderSource.substr(begin, eol - begin);

        size_t nextLinePos = InShaderSource.find_first_not_of("\r\n", eol); // Start of shader code after shader type declaration line
        AE_ASSERT(nextLinePos != String::npos, "Syntax error");
        pos = InShaderSource.find(typeToken, nextLinePos); // Start of next shader type declaration line

        shaderSources[type] = (pos == String::npos) ? InShaderSource.substr(nextLinePos) : InShaderSource.substr(nextLinePos, pos - nextLinePos);
    }

    return shaderSources;
}