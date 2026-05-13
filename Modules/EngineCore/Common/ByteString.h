#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Types.h"
#include "ByteString.gen.h"

/* A ByteString owns a string of binary data and provides methods for manipulating it */
class ByteString : public Object {
public:
    ARTIFACT_CLASS();

    ByteString(size_t InSizeInBytes, byte* InData);
    virtual ~ByteString();

    size_t GetSizeInBytes() const { return m_SizeInBytes; }
    const byte* GetData() const { return m_Data; }

private:
    size_t m_SizeInBytes;
    byte* m_Data;
};