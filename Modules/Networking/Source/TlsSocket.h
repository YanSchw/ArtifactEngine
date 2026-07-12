#pragma once
#include "NetworkStream.h"
#include "IpEndpoint.h"
#include "TlsSocket.gen.h"

/* Client-side TLS stream layered on top of a TcpSocket. The concrete implementation is
 * supplied by the active platform module using its native TLS provider. */
class TlsSocket : public NetworkStream {
public:
    ARTIFACT_CLASS();

    static SharedObjectPtr<TlsSocket> Create();

    /** Connects to the peer and performs the TLS handshake. InHostname drives SNI and
     *  certificate/hostname validation against the system trust store. */
    virtual bool Connect(const IpEndpoint& InPeer, const String& InHostname) = 0;
};
