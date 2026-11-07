#include "StaticMeshPass.h"
#include "Assets/Material.h"
#include "Assets/Mesh.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderStage.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderData.h"
#include "Rendering/VertexBuffer.h"
#include "GameFramework/World.h"
#include "GameFramework/StaticMeshNode.h"

void StaticMeshPass::Execute(const RenderContext& InContext) {
    if (!InContext.World) {
        return;
    }

    FrameBuffer& target = *GetStage()->GetTarget();
    RenderCommandQueue& renderQueue = RenderingAPI::GetInstance()->GetRenderQueue();
    for (Node* node : InContext.World->GetAllNodes()) {
        StaticMeshNode* staticMesh = node->As<StaticMeshNode>();
        Mesh* mesh = staticMesh ? staticMesh->GetMesh() : nullptr;
        if (!mesh || !staticMesh->IsEnabled()) {
            continue;
        }

        VertexBuffer* vertexBuffer = mesh->GetVertexBuffer();
        AE_ASSERT(vertexBuffer);
        ShaderData* shaderData = staticMesh->GetPerMeshShaderData();
        const Array<SubMesh>& subMeshes = mesh->GetSubMeshes();
        for (int32_t slot = 0; slot < subMeshes.Size(); slot++) {
            Material* material = staticMesh->GetMaterial(slot);
            if (!material || !material->IsReadyToRender() ||
                material->GetShader()->GetRenderState().Path != GetRenderPath()) {
                continue;
            }
            Pipeline* pipeline = m_Pipelines.Resolve(*material, target, InContext);
            if (!pipeline) {
                continue;
            }
            pipeline->Bind();
            renderQueue.Push(RenderCommandType::SetShaderData, CmdSetShaderData{ shaderData });
            vertexBuffer->Draw(subMeshes[slot].IndexCount, subMeshes[slot].FirstIndex);
        }
    }
}
