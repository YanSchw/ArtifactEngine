#pragma once
#include "CoreMinimal.h"
#include "ShaderData.gen.h"

/*
 * Represents the transient block of data consumed by a shader for a draw call.
 */
class ShaderData : public Object {
public:
    ARTIFACT_CLASS();

    template<typename T>
    void Set(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>);

        m_Data.Resize(sizeof(T));
        memcpy(m_Data.Data(), &value, sizeof(T));
    }

    const void* Data() const {
        return m_Data.IsEmpty() ? nullptr : m_Data.Data();
    }

    size_t Size() const {
        return m_Data.Size();
    }

private:
    Array<byte> m_Data;
};