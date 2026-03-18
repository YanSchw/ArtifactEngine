#pragma once
#include <cstdint>
#include "Common/Types.h"

enum class ResourceType : uint8_t {
    None = 0,
    Shader,
    COUNT
};

class Resource {
public:
    Resource(ResourceType type, size_t size, byte* data)
        : m_Type(type), m_SizeInBytes(size), m_Data(data) { }

    ResourceType m_Type = ResourceType::None;
    size_t m_SizeInBytes = 0;
    byte* m_Data = nullptr;
};