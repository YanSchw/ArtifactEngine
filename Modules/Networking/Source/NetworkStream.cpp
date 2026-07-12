#include "NetworkStream.h"

bool NetworkStream::SendAll(const void* InData, size_t InSize) {
    const byte* cursor = static_cast<const byte*>(InData);
    size_t remaining = InSize;
    while (remaining > 0) {
        int32_t sent = Send(cursor, remaining);
        if (sent <= 0)
            return false;
        cursor += sent;
        remaining -= static_cast<size_t>(sent);
    }
    return true;
}
