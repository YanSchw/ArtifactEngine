#pragma once
#include "CoreMinimal.h"
#include "Common/ByteString.h"
#include "Buffer.gen.h"

class ShaderBuffer : public Object {
public:
    ARTIFACT_CLASS();

    virtual ~ShaderBuffer() { }
    void UploadData(const ByteString& InData);
    virtual void* MapData(size_t InSize, size_t InOffset = 0) = 0;
    virtual void UnmapData() = 0;
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