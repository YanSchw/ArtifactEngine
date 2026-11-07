#include "DebugDrawPass.h"
#include "Rendering/RenderStage.h"
#include "GameFramework/DebugDraw.h"

void DebugDrawPass::Execute(const RenderContext& InContext) {
    DebugDraw::Render(InContext.World, GetStage()->GetTarget(), InContext.Camera);
}
