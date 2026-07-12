#pragma once
#include "NetworkStream.h"
#include "IpEndpoint.h"
#include "TcpSocket.gen.h"

/* Reliable byte-stream socket. */
class TcpSocket : public NetworkStream {
public:
    ARTIFACT_CLASS();

    static SharedObjectPtr<TcpSocket> Create();

    virtual bool Connect(const IpEndpoint& InPeer) = 0;

    virtual bool Listen(uint16_t InPort, const String& InAddress = "0.0.0.0", int32_t InBacklog = 16) = 0;
    /** Blocks for the next incoming connection, returning it as its own socket. */
    virtual SharedObjectPtr<TcpSocket> Accept() = 0;

    virtual void SetBlocking(bool InBlocking) = 0;
    virtual void SetNoDelay(bool InEnabled) = 0;

    virtual IpEndpoint GetPeerEndpoint() const = 0;
    virtual uint16_t GetLocalPort() const = 0;
};
