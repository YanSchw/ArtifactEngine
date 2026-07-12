#pragma once
#include "UdpSocket.h"
#include "TcpSocket.h"
#include "Win64Sockets.gen.h"

class Win64UdpSocket : public UdpSocket {
public:
    ARTIFACT_CLASS();
    Win64UdpSocket() = default;
    virtual ~Win64UdpSocket();

    virtual bool Bind(uint16_t InPort, const String& InAddress) override;
    virtual bool Connect(const IpEndpoint& InPeer) override;
    virtual int32_t SendTo(const void* InData, size_t InSize, const IpEndpoint& InTo) override;
    virtual int32_t ReceiveFrom(void* OutData, size_t InSize, IpEndpoint& OutFrom) override;
    virtual int32_t Send(const void* InData, size_t InSize) override;
    virtual int32_t Receive(void* OutData, size_t InSize) override;
    virtual void SetBlocking(bool InBlocking) override;
    virtual void SetBroadcastEnabled(bool InEnabled) override;
    virtual uint16_t GetLocalPort() const override;
    virtual bool IsValid() const override;
    virtual void Close() override;

private:
    bool EnsureSocket();
    int64_t m_Handle = -1;
};

class Win64TcpSocket : public TcpSocket {
public:
    ARTIFACT_CLASS();
    Win64TcpSocket() = default;
    virtual ~Win64TcpSocket();

    virtual bool Connect(const IpEndpoint& InPeer) override;
    virtual bool Listen(uint16_t InPort, const String& InAddress, int32_t InBacklog) override;
    virtual SharedObjectPtr<TcpSocket> Accept() override;
    virtual int32_t Send(const void* InData, size_t InSize) override;
    virtual int32_t Receive(void* OutData, size_t InSize) override;
    virtual void SetBlocking(bool InBlocking) override;
    virtual void SetNoDelay(bool InEnabled) override;
    virtual IpEndpoint GetPeerEndpoint() const override;
    virtual uint16_t GetLocalPort() const override;
    virtual bool IsValid() const override;
    virtual void Close() override;

private:
    bool EnsureSocket();
    int64_t m_Handle = -1;
};
