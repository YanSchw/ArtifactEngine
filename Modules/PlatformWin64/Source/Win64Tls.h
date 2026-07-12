#pragma once
#include "TlsSocket.h"
#include "TcpSocket.h"
#include "Win64Tls.gen.h"

class Win64TlsSocket : public TlsSocket {
public:
    ARTIFACT_CLASS();
    Win64TlsSocket() = default;
    virtual ~Win64TlsSocket();

    virtual bool Connect(const IpEndpoint& InPeer, const String& InHostname) override;
    virtual int32_t Send(const void* InData, size_t InSize) override;
    virtual int32_t Receive(void* OutData, size_t InSize) override;
    virtual bool IsValid() const override;
    virtual void Close() override;

private:
    SharedObjectPtr<TcpSocket> m_Transport;
    struct SChannelState* m_State = nullptr;
};
