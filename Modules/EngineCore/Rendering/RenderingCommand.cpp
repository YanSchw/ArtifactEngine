#include "RenderingCommand.h"

RenderCommandQueue::RenderCommandQueue() {
    commands.reserve(1024);
}

void RenderCommandQueue::Clear() {
    commands.clear();
    commands.reserve(1024);
}
