#include "Buffer.h"
#include "RenderingAPI.h"

#include <cstring>

SharedObjectPtr<UniformBuffer> UniformBuffer::Create(uint32_t InBinding, size_t InSize) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateUniformBuffer(InBinding, InSize);
}

SharedObjectPtr<StorageBuffer> StorageBuffer::Create(uint32_t InBinding, size_t InSize) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    if (!RenderingAPI::GetInstance()->GetCapabilities().StorageBuffers) {
        AE_ERROR("The active rendering API does not support storage buffers");
        return nullptr;
    }
    return RenderingAPI::GetInstance()->CreateStorageBuffer(InBinding, InSize);
}

void ShaderBuffer::UploadData(const ByteString& InData) {
    UploadData(InData.GetData(), InData.GetSizeInBytes());
}

void ShaderBuffer::UploadData(const void* InData, size_t InSize, size_t InOffset) {
    if (InOffset + InSize > m_Size) {
        AE_ERROR("Buffer write of {0} bytes at offset {1} exceeds its size of {2}", InSize, InOffset, m_Size);
        return;
    }
    if (void* mapped = MapData(InSize, InOffset)) {
        memcpy(mapped, InData, InSize);
        UnmapData();
    }
}

void ShaderBuffer::ReadData(void* OutData, size_t InSize, size_t InOffset) {
    if (InOffset + InSize > m_Size) {
        AE_ERROR("Buffer read of {0} bytes at offset {1} exceeds its size of {2}", InSize, InOffset, m_Size);
        return;
    }
    if (const void* mapped = MapData(InSize, InOffset)) {
        memcpy(OutData, mapped, InSize);
        UnmapData();
    }
}