#pragma once
#include "CoreMinimal.h"
#include <variant>
#include <vector>

enum class RenderCommandType {
    BindVertexBuffer,
    DrawIndexed
};

struct CmdBindVertexBuffer {
    WeakObjectPtr<class VertexBuffer> Buffer;
};

struct CmdDrawIndexed {
    uint32_t IndexCount;
    uint32_t FirstIndex;
    int32_t  VertexOffset;
};

using RenderCommandData = std::variant<
    CmdBindVertexBuffer,
    CmdDrawIndexed
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