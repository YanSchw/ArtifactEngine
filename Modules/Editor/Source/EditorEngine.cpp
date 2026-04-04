#include "EditorEngine.h"
#include "CoreMinimal.h"
#include "Core/Platform.h"
#include "Platform/FileIO.h"

#include "Window.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/Buffer.h"
#include "Rendering/Pipeline.h"

SharedObjectPtr<Window> s_Window;

SharedObjectPtr<VertexBuffer> s_VertexBuffer1;
SharedObjectPtr<VertexBuffer> s_VertexBuffer2;

SharedObjectPtr<Shader> s_Shader;
SharedObjectPtr<UniformBuffer> s_UniformBuffer;
SharedObjectPtr<Pipeline> s_Pipeline;

struct {
    glm::mat4 transformationMatrix;
} uniformBufferData;

void UpdateUniformData() {
    static auto timeStart = std::chrono::high_resolution_clock::now(); // Initialize once

    auto timeNow = std::chrono::high_resolution_clock::now();
    long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - timeStart).count();
    float angle = (millis % 4000) / 4000.0f * glm::radians(360.f);

    // Proper identity initialization
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0, 0, 1));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f / 3.0f, -0.5f / 3.0f, 0.0f));

    // View and projection
    auto viewMatrix = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
    auto projMatrix = glm::perspective(glm::radians(70.f), s_Window->GetWidth() / (float)s_Window->GetHeight(), 0.1f, 10.0f);

    uniformBufferData.transformationMatrix = projMatrix * viewMatrix * modelMatrix;

    // Upload
    void* data = s_UniformBuffer->MapData(sizeof(uniformBufferData), 0);
    memcpy(data, &uniformBufferData, sizeof(uniformBufferData));
    s_UniformBuffer->UnmapData();
}

void EditorEngine::Initialize() {
    s_Window = Window::Create(WindowParams{ "Artifact Editor", 1280, 720 });
    AE_ASSERT(s_Window);

    Object::Create(Platform::GetDefaultRenderingAPIClass());
    AE_ASSERT(RenderingAPI::GetInstance(), "Failed to create RenderingAPI instance!");
    RenderingAPI::GetInstance()->Initialize();

    Array<Vertex> vertices1 = {
        { { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    s_VertexBuffer1 = VertexBuffer::Create(vertices1, { 0, 1, 2 });
    Array<Vertex> vertices2 = {
        { { -0.5f, -0.5f,  -1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f,  -1.0f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  -1.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    s_VertexBuffer2 = VertexBuffer::Create(vertices2, { 0, 1, 2 });

    s_Shader = Shader::Create(FileIO::ReadFileToString("/Users/yannick/Developer/ArtifactEngine/Content/Shaders/Shader.glsl"));
    s_UniformBuffer = UniformBuffer::Create(0, sizeof(uniformBufferData));
    PipelineDesc pipelineDesc;
    pipelineDesc.Shader = s_Shader;
    pipelineDesc.Buffers.Add(s_UniformBuffer);
    s_Pipeline = Pipeline::Create(pipelineDesc);
}

bool EditorEngine::MainTick(double InDeltaTime) {
    UpdateUniformData();
    s_Pipeline->Bind();
    s_VertexBuffer1->Draw();
    s_VertexBuffer2->Draw();

    RenderingAPI::GetInstance()->Draw();

    s_Window->PollEvents();
    return !s_Window->ShouldClose();
}

void EditorEngine::Shutdown() {
    s_Window = nullptr;
    RenderingAPI::GetInstance()->CleanUp(true);
}