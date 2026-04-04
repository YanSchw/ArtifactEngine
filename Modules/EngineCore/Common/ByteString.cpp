#include "ByteString.h"

ByteString::ByteString(size_t InSizeInBytes, byte* InData) {
    m_SizeInBytes = InSizeInBytes;
    m_Data = InData;
}

ByteString::~ByteString() {
    delete[] m_Data;
}
