#include "Buffer.h"
#include "RenderingAPI.h"

SharedObjectPtr<UniformBuffer> UniformBuffer::Create(uint32_t InBinding, size_t InSize) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateUniformBuffer(InBinding, InSize);
}

SharedObjectPtr<StorageBuffer> StorageBuffer::Create(uint32_t InBinding, size_t InSize) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateStorageBuffer(InBinding, InSize);
}

void ShaderBuffer::UploadData(const ByteString& InData) {
    void* mapped = MapData(InData.GetSizeInBytes(), 0);
    if (mapped) {
        memcpy(mapped, InData.GetData(), InData.GetSizeInBytes());
        UnmapData();
    }
}