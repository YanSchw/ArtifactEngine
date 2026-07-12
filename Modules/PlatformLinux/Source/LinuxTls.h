#pragma once
#include "TlsSocket.h"
#include "TcpSocket.h"
#include "LinuxTls.gen.h"

class LinuxTlsSocket : public TlsSocket {
public:
    ARTIFACT_CLASS();
    LinuxTlsSocket() = default;
    virtual ~LinuxTlsSocket();

    virtual bool Connect(const IpEndpoint& InPeer, const String& InHostname) override;
    virtual int32_t Send(const void* InData, size_t InSize) override;
    virtual int32_t Receive(void* OutData, size_t InSize) override;
    virtual bool IsValid() const override;
    virtual void Close() override;

private:
    SharedObjectPtr<TcpSocket> m_Transport;
    void* m_Ssl = nullptr;
    void* m_Context = nullptr;
};
