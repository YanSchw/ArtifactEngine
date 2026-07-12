#pragma once
#include "CoreMinimal.h"
#include "IpEndpoint.h"
#include "UdpSocket.gen.h"

/* Connectionless datagram socket. */
class UdpSocket : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~UdpSocket() = default;

    static SharedObjectPtr<UdpSocket> Create();

    virtual bool Bind(uint16_t InPort, const String& InAddress = "0.0.0.0") = 0;

    /** Fixes a default peer so Send/Receive can be used without an endpoint. */
    virtual bool Connect(const IpEndpoint& InPeer) = 0;

    virtual int32_t SendTo(const void* InData, size_t InSize, const IpEndpoint& InTo) = 0;
    virtual int32_t ReceiveFrom(void* OutData, size_t InSize, IpEndpoint& OutFrom) = 0;

    virtual int32_t Send(const void* InData, size_t InSize) = 0;
    virtual int32_t Receive(void* OutData, size_t InSize) = 0;

    int32_t SendTo(const String& InData, const IpEndpoint& InTo) { return SendTo(InData.data(), InData.size(), InTo); }
    int32_t Send(const String& InData) { return Send(InData.data(), InData.size()); }

    virtual void SetBlocking(bool InBlocking) = 0;
    virtual void SetBroadcastEnabled(bool InEnabled) = 0;
    virtual uint16_t GetLocalPort() const = 0;

    virtual bool IsValid() const = 0;
    virtual void Close() = 0;
};
