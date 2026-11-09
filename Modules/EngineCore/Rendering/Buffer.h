#pragma once
#include "CoreMinimal.h"
#include "Common/ByteString.h"
#include "Buffer.gen.h"

class ShaderBuffer : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~ShaderBuffer() { }

    uint32_t GetBinding() const { return m_Binding; }
    size_t GetSize() const { return m_Size; }

    void UploadData(const ByteString& InData);
    void UploadData(const void* InData, size_t InSize, size_t InOffset = 0);
    /** Only sees GPU writes from frames that have finished, see RenderingAPI::WaitIdle. */
    void ReadData(void* OutData, size_t InSize, size_t InOffset = 0);

    virtual void* MapData(size_t InSize, size_t InOffset = 0) = 0;
    virtual void UnmapData() = 0;

protected:
    uint32_t m_Binding = 0;
    size_t m_Size = 0;
};

class UniformBuffer : public ShaderBuffer {
public:
    ARTIFACT_CLASS();

    virtual ~UniformBuffer() { }

    static SharedObjectPtr<UniformBuffer> Create(uint32_t InBinding, size_t InSize);

};

class StorageBuffer : public ShaderBuffer {
public:
    ARTIFACT_CLASS();

    virtual ~StorageBuffer() { }

    static SharedObjectPtr<StorageBuffer> Create(uint32_t InBinding, size_t InSize);
};
