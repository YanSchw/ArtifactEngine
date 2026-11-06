#pragma once
#include "CoreMinimal.h"
#include <variant>
#include <vector>

enum class RenderCommandType {
    BeginRenderPass,
    BindPipeline,
    BindVertexBuffer,
    DrawIndexed,
    SetShaderData,
    BindComputePipeline,
    Dispatch
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

struct CmdBindComputePipeline {
    WeakObjectPtr<class ComputePipeline> Pipeline;
};

struct CmdDispatch {
    uint32_t GroupCountX;
    uint32_t GroupCountY;
    uint32_t GroupCountZ;
};

using RenderCommandData = std::variant<
    CmdBeginRenderPass,
    CmdBindPipeline,
    CmdBindVertexBuffer,
    CmdDrawIndexed,
    CmdSetShaderData,
    CmdBindComputePipeline,
    CmdDispatch
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
        Track(commands.back());
    }

    void Clear();

    /** The target of the render pass the next draw lands in, or null once a compute pipeline ended it. */
    Object* GetOpenRenderTarget() const { return m_OpenRenderTarget.Get(); }
    class ComputePipeline* GetBoundComputePipeline() const { return m_BoundComputePipeline.Get(); }

    std::vector<RenderCommand> commands;

private:
    void Track(const RenderCommand& InCommand);

    WeakObjectPtr<Object> m_OpenRenderTarget;
    WeakObjectPtr<class ComputePipeline> m_BoundComputePipeline;
};
