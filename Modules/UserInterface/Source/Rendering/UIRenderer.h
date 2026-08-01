#pragma once
#include "Common/Types.h"
#include "Common/Map.h"
#include "Object/Pointer.h"
#include "GameFramework/UINode.h"

class Surface;
class Shader;
class Sampler;
class Texture;
class Pipeline;
class VertexBuffer;
class UniformBuffer;
class UICanvas;
class UIDrawList;

/** Draws a UI canvas each frame.
 *  The engine owns one instance and calls Render() after the scene blit, before
 *  RenderingAPI::Draw(). */
class UIRenderer {
public:
    UIRenderer() = default;

    void Render(Object* InTarget, UICanvas* InCanvas, const Vec2& InViewportSize, const UIFrameContext& InContext);
    void Submit(Object* InTarget, const Vec2& InViewportSize, const UIDrawList& InDrawList, const Mat4& InProjection);

private:
    void CreateSharedResources();
    void CreatePipelines(Object* InTarget);
    Pipeline* GetImagePipeline(Texture* InTexture);

    SharedObjectPtr<Shader> m_SolidShader;
    SharedObjectPtr<Shader> m_TextShader;
    SharedObjectPtr<Shader> m_ImageShader;
    SharedObjectPtr<Sampler> m_Sampler;
    SharedObjectPtr<Texture> m_WhiteTexture;
    SharedObjectPtr<UniformBuffer> m_ProjectionBuffer;
    SharedObjectPtr<Pipeline> m_SolidPipeline;
    SharedObjectPtr<Pipeline> m_TextPipeline;
    Map<Texture*, SharedObjectPtr<Pipeline>> m_ImagePipelines;
    SharedObjectPtr<VertexBuffer> m_DynamicVertexBuffer;

    bool m_ResourcesReady = false;
    Object* m_CachedTarget = nullptr;
    uint32_t m_CachedWidth = 0;
    uint32_t m_CachedHeight = 0;
    bool m_TextPipelineReady = false;
};
