#include "ShaderCompiler.h"

static Array<SharedObjectPtr<ShaderCompiler>> s_Compilers;
static bool s_Discovered = false;

static void DiscoverCompilers() {
    if (s_Discovered) {
        return;
    }
    s_Discovered = true;

    for (const Class& compilerClass : Class::GetSubclassesOf(ShaderCompiler::StaticClass())) {
        if (compilerClass == ShaderCompiler::StaticClass()) {
            continue;
        }
        if (ShaderCompiler* compiler = Object::Create<ShaderCompiler>(compilerClass)) {
            s_Compilers.Add(compiler);
        }
    }
}

ShaderCompiler* ShaderCompiler::Get(ShaderAPI InAPI) {
    DiscoverCompilers();

    for (const SharedObjectPtr<ShaderCompiler>& compiler : s_Compilers) {
        if (compiler->GetAPI() == InAPI) {
            return compiler.Get();
        }
    }
    return nullptr;
}

Array<ShaderAPI> ShaderCompiler::GetAPIsForPlatform(PlatformType InPlatform) {
    DiscoverCompilers();

    Array<ShaderAPI> apis;
    for (const SharedObjectPtr<ShaderCompiler>& compiler : s_Compilers) {
        if (compiler->SupportsPlatform(InPlatform) && !apis.Contains(compiler->GetAPI())) {
            apis.Add(compiler->GetAPI());
        }
    }
    return apis;
}
