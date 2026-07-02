#pragma once
#include "Common/Types.h"
#include "Object/Pointer.h"
#include "GameFramework/UINode.h"

class Surface;
class Shader;
class Sampler;
class Texture;
class Pipeline;
class VertexBuffer;
class UniformBuffer;

/** Draws a UI tree each frame via two overlay pipelines (solid + SDF text) that target the window
 *  surface with alpha blending on and depth test off. Text uses UINode::GetDefaultFont(). The
 *  engine owns one instance and calls Render() after the scene blit, before RenderingAPI::Draw(). */
class UIRenderer {
public:
    UIRenderer() = default;

    void Render(Surface* InTarget, UINode* InRoot, const Vec2& InViewportSize, const UIFrameContext& InContext);

private:
    void CreateSharedResources();
    void CreatePipelines(Surface* InTarget);

    SharedObjectPtr<Shader> m_SolidShader;
    SharedObjectPtr<Shader> m_TextShader;
    SharedObjectPtr<Sampler> m_Sampler;
    SharedObjectPtr<Texture> m_WhiteTexture;
    SharedObjectPtr<UniformBuffer> m_ProjectionBuffer;
    SharedObjectPtr<Pipeline> m_SolidPipeline;
    SharedObjectPtr<Pipeline> m_TextPipeline;
    SharedObjectPtr<VertexBuffer> m_SolidVertexBuffer;
    SharedObjectPtr<VertexBuffer> m_TextVertexBuffer;

    bool m_ResourcesReady = false;
    Surface* m_CachedTarget = nullptr;
    uint32_t m_CachedWidth = 0;
    uint32_t m_CachedHeight = 0;
    bool m_TextPipelineReady = false;
};
