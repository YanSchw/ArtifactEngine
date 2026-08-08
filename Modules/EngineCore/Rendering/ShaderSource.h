#pragma once
#include "CoreMinimal.h"
#include "Common/Array.h"
#include "Common/Map.h"
#include "ShaderSource.gen.h"

ARTIFACT_ENUM();
enum class ShaderStage : uint32_t {
    Vertex,
    Fragment,
    Compute
};

ARTIFACT_ENUM();
enum class BlendMode : uint32_t {
    Opaque,
    Alpha,
    Additive,
    Multiply
};

ARTIFACT_ENUM();
enum class CullMode : uint32_t {
    Back,
    Front,
    None
};

ARTIFACT_ENUM();
enum class DepthMode : uint32_t {
    None,
    Test,
    Write,
    TestWrite
};

ARTIFACT_ENUM();
enum class WindingOrder : uint32_t {
    CounterClockwise,
    Clockwise
};

struct ShaderRenderState {
    ARTIFACT_STRUCT();

    BlendMode Blend = BlendMode::Opaque;
    CullMode Cull = CullMode::Back;
    DepthMode Depth = DepthMode::TestWrite;
    WindingOrder FrontFace = WindingOrder::CounterClockwise;
};

struct ShaderSourceLocation {
    String File;
    uint32_t Line = 0;
};

struct ShaderStageSource {
    ShaderStage Stage = ShaderStage::Vertex;
    String Source;
    Array<ShaderSourceLocation> LineMap;

    ShaderSourceLocation ResolveLine(uint32_t InGeneratedLine) const;
};

class ShaderSource {
public:
    static bool Preprocess(const String& InPath, ShaderSource& OutSource, String& OutError);
    static bool PreprocessSource(const String& InPath, const String& InSource, ShaderSource& OutSource, String& OutError);

    const String& GetPath() const { return m_Path; }
    const ShaderRenderState& GetRenderState() const { return m_RenderState; }
    const Array<ShaderStageSource>& GetStages() const { return m_Stages; }

    const ShaderStageSource* FindStage(ShaderStage InStage) const;
    bool HasStage(ShaderStage InStage) const { return FindStage(InStage) != nullptr; }

    static String GetStageName(ShaderStage InStage);
    static bool ParseStageName(const String& InName, ShaderStage& OutStage);
    static bool DeclaresStage(const String& InSource);

private:
    String m_Path;
    ShaderRenderState m_RenderState;
    Array<ShaderStageSource> m_Stages;
    Array<String> m_Dependencies;
};
