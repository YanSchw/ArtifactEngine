#include "MacOSSockets.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

namespace {
    bool ResolveEndpoint(const IpEndpoint& InEndpoint, int InType, sockaddr_in& OutAddress) {
        addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = InType;

        addrinfo* results = nullptr;
        if (getaddrinfo(InEndpoint.Address.c_str(), std::to_string(InEndpoint.Port).c_str(), &hints, &results) != 0 || !results)
            return false;

        OutAddress = *reinterpret_cast<sockaddr_in*>(results->ai_addr);
        freeaddrinfo(results);
        return true;
    }

    IpEndpoint EndpointFromSockaddr(const sockaddr_in& InAddress) {
        char text[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &InAddress.sin_addr, text, sizeof(text));
        return IpEndpoint(text, ntohs(InAddress.sin_port));
    }

    uint16_t BoundPort(int InSocket) {
        sockaddr_in address = {};
        socklen_t length = sizeof(address);
        if (getsockname(InSocket, reinterpret_cast<sockaddr*>(&address), &length) != 0)
            return 0;
        return ntohs(address.sin_port);
    }

    void SetSocketBlocking(int InSocket, bool InBlocking) {
        int flags = fcntl(InSocket, F_GETFL, 0);
        if (flags == -1)
            return;
        fcntl(InSocket, F_SETFL, InBlocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK));
    }

    void SuppressSigPipe(int InSocket) {
        int value = 1;
        setsockopt(InSocket, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
    }
}

MacOSUdpSocket::~MacOSUdpSocket() { Close(); }

bool MacOSUdpSocket::EnsureSocket() {
    if (m_Handle != -1)
        return true;
    m_Handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_Handle != -1)
        SuppressSigPipe(static_cast<int>(m_Handle));
    return m_Handle != -1;
}

bool MacOSUdpSocket::Bind(uint16_t InPort, const String& InAddress) {
    if (!EnsureSocket())
        return false;
    sockaddr_in address = {};
    if (!ResolveEndpoint({ InAddress, InPort }, SOCK_DGRAM, address))
        return false;
    return bind(static_cast<int>(m_Handle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

bool MacOSUdpSocket::Connect(const IpEndpoint& InPeer) {
    if (!EnsureSocket())
        return false;
    sockaddr_in address = {};
    if (!ResolveEndpoint(InPeer, SOCK_DGRAM, address))
        return false;
    return connect(static_cast<int>(m_Handle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

int32_t MacOSUdpSocket::SendTo(const void* InData, size_t InSize, const IpEndpoint& InTo) {
    if (!EnsureSocket())
        return -1;
    sockaddr_in address = {};
    if (!ResolveEndpoint(InTo, SOCK_DGRAM, address))
        return -1;
    return sendto(static_cast<int>(m_Handle), InData, InSize, 0, reinterpret_cast<sockaddr*>(&address), sizeof(address));
}

int32_t MacOSUdpSocket::ReceiveFrom(void* OutData, size_t InSize, IpEndpoint& OutFrom) {
    if (m_Handle == -1)
        return -1;
    sockaddr_in address = {};
    socklen_t length = sizeof(address);
    int32_t received = recvfrom(static_cast<int>(m_Handle), OutData, InSize, 0, reinterpret_cast<sockaddr*>(&address), &length);
    if (received >= 0)
        OutFrom = EndpointFromSockaddr(address);
    return received;
}

int32_t MacOSUdpSocket::Send(const void* InData, size_t InSize) {
    if (m_Handle == -1)
        return -1;
    return send(static_cast<int>(m_Handle), InData, InSize, 0);
}

int32_t MacOSUdpSocket::Receive(void* OutData, size_t InSize) {
    if (m_Handle == -1)
        return -1;
    return recv(static_cast<int>(m_Handle), OutData, InSize, 0);
}

void MacOSUdpSocket::SetBlocking(bool InBlocking) {
    if (EnsureSocket())
        SetSocketBlocking(static_cast<int>(m_Handle), InBlocking);
}

void MacOSUdpSocket::SetBroadcastEnabled(bool InEnabled) {
    if (!EnsureSocket())
        return;
    int value = InEnabled ? 1 : 0;
    setsockopt(static_cast<int>(m_Handle), SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
}

uint16_t MacOSUdpSocket::GetLocalPort() const {
    return m_Handle == -1 ? 0 : BoundPort(static_cast<int>(m_Handle));
}

bool MacOSUdpSocket::IsValid() const { return m_Handle != -1; }

void MacOSUdpSocket::Close() {
    if (m_Handle != -1) {
        close(static_cast<int>(m_Handle));
        m_Handle = -1;
    }
}

MacOSTcpSocket::~MacOSTcpSocket() { Close(); }

bool MacOSTcpSocket::EnsureSocket() {
    if (m_Handle != -1)
        return true;
    m_Handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_Handle != -1)
        SuppressSigPipe(static_cast<int>(m_Handle));
    return m_Handle != -1;
}

bool MacOSTcpSocket::Connect(const IpEndpoint& InPeer) {
    if (!EnsureSocket())
        return false;
    sockaddr_in address = {};
    if (!ResolveEndpoint(InPeer, SOCK_STREAM, address))
        return false;
    return connect(static_cast<int>(m_Handle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

bool MacOSTcpSocket::Listen(uint16_t InPort, const String& InAddress, int32_t InBacklog) {
    if (!EnsureSocket())
        return false;
    int reuse = 1;
    setsockopt(static_cast<int>(m_Handle), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address = {};
    if (!ResolveEndpoint({ InAddress, InPort }, SOCK_STREAM, address))
        return false;
    if (bind(static_cast<int>(m_Handle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        return false;
    return listen(static_cast<int>(m_Handle), InBacklog) == 0;
}

SharedObjectPtr<TcpSocket> MacOSTcpSocket::Accept() {
    if (m_Handle == -1)
        return nullptr;
    int client = accept(static_cast<int>(m_Handle), nullptr, nullptr);
    if (client == -1)
        return nullptr;
    MacOSTcpSocket* connection = new MacOSTcpSocket();
    connection->m_Handle = client;
    SuppressSigPipe(client);
    return SharedObjectPtr<TcpSocket>(connection);
}

int32_t MacOSTcpSocket::Send(const void* InData, size_t InSize) {
    if (m_Handle == -1)
        return -1;
    return send(static_cast<int>(m_Handle), InData, InSize, 0);
}

int32_t MacOSTcpSocket::Receive(void* OutData, size_t InSize) {
    if (m_Handle == -1)
        return -1;
    return recv(static_cast<int>(m_Handle), OutData, InSize, 0);
}

void MacOSTcpSocket::SetBlocking(bool InBlocking) {
    if (EnsureSocket())
        SetSocketBlocking(static_cast<int>(m_Handle), InBlocking);
}

void MacOSTcpSocket::SetNoDelay(bool InEnabled) {
    if (!EnsureSocket())
        return;
    int value = InEnabled ? 1 : 0;
    setsockopt(static_cast<int>(m_Handle), IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
}

IpEndpoint MacOSTcpSocket::GetPeerEndpoint() const {
    if (m_Handle == -1)
        return {};
    sockaddr_in address = {};
    socklen_t length = sizeof(address);
    if (getpeername(static_cast<int>(m_Handle), reinterpret_cast<sockaddr*>(&address), &length) != 0)
        return {};
    return EndpointFromSockaddr(address);
}

uint16_t MacOSTcpSocket::GetLocalPort() const {
    return m_Handle == -1 ? 0 : BoundPort(static_cast<int>(m_Handle));
}

bool MacOSTcpSocket::IsValid() const { return m_Handle != -1; }

void MacOSTcpSocket::Close() {
    if (m_Handle != -1) {
        close(static_cast<int>(m_Handle));
        m_Handle = -1;
    }
}
