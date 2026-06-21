#pragma once
#include "CoreMinimal.h"
#include <variant>
#include <vector>

enum class RenderCommandType {
    BeginRenderPass,
    BindPipeline,
    BindVertexBuffer,
    DrawIndexed,
    SetShaderData
};

struct CmdBeginRenderPass {
    WeakObjectPtr<class Object> Target;
};

struct CmdBindPipeline {
    WeakObjectPtr<class Pipeline> Pipeline;
};

struct CmdBindVertexBuffer {
    WeakObjectPtr<class VertexBuffer> Buffer;
};

struct CmdDrawIndexed {
    uint32_t IndexCount;
    uint32_t FirstIndex;
    int32_t  VertexOffset;
};

struct CmdSetShaderData {
    WeakObjectPtr<class ShaderData> Data;
};

using RenderCommandData = std::variant<
    CmdBeginRenderPass,
    CmdBindPipeline,
    CmdBindVertexBuffer,
    CmdDrawIndexed,
    CmdSetShaderData
>;

struct RenderCommand {
    RenderCommandType Type;
    RenderCommandData Data;
};

class RenderCommandQueue {
public:
    RenderCommandQueue();
    
    template<typename T>
    void Push(RenderCommandType type, const T& cmd) {
        commands.push_back({ type, cmd });
    }

    void Clear();

    std::vector<RenderCommand> commands;
};