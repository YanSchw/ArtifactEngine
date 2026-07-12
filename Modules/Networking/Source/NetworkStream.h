#pragma once
#include "CoreMinimal.h"
#include "NetworkStream.gen.h"

/* Reliable, ordered byte stream. Base for both plain TCP (TcpSocket) and encrypted
 * transports (TlsSocket) so higher-level protocols can be written against either. */
class NetworkStream : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~NetworkStream() = default;

    virtual int32_t Send(const void* InData, size_t InSize) = 0;
    virtual int32_t Receive(void* OutData, size_t InSize) = 0;

    /** Sends the whole buffer, looping until everything is written or the peer drops. */
    bool SendAll(const void* InData, size_t InSize);

    int32_t Send(const String& InData) { return Send(InData.data(), InData.size()); }
    bool SendAll(const String& InData) { return SendAll(InData.data(), InData.size()); }

    virtual bool IsValid() const = 0;
    virtual void Close() = 0;
};
