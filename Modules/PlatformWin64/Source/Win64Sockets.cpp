#include "Win64Sockets.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

namespace {
    void EnsureWinsock() {
        struct WinsockGuard {
            WinsockGuard() { WSADATA data; WSAStartup(MAKEWORD(2, 2), &data); }
            ~WinsockGuard() { WSACleanup(); }
        };
        static WinsockGuard s_Guard;
    }

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

    uint16_t BoundPort(SOCKET InSocket) {
        sockaddr_in address = {};
        int length = sizeof(address);
        if (getsockname(InSocket, reinterpret_cast<sockaddr*>(&address), &length) != 0)
            return 0;
        return ntohs(address.sin_port);
    }

    void SetSocketBlocking(SOCKET InSocket, bool InBlocking) {
        u_long mode = InBlocking ? 0 : 1;
        ioctlsocket(InSocket, FIONBIO, &mode);
    }
}

Win64UdpSocket::~Win64UdpSocket() { Close(); }

bool Win64UdpSocket::EnsureSocket() {
    if (m_Handle != -1)
        return true;
    EnsureWinsock();
    SOCKET handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    m_Handle = handle == INVALID_SOCKET ? -1 : static_cast<int64_t>(handle);
    return m_Handle != -1;
}

bool Win64UdpSocket::Bind(uint16_t InPort, const String& InAddress) {
    if (!EnsureSocket())
        return false;
    sockaddr_in address = {};
    if (!ResolveEndpoint({ InAddress, InPort }, SOCK_DGRAM, address))
        return false;
    return bind(static_cast<SOCKET>(m_Handle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

bool Win64UdpSocket::Connect(const IpEndpoint& InPeer) {
    if (!EnsureSocket())
        return false;
    sockaddr_in address = {};
    if (!ResolveEndpoint(InPeer, SOCK_DGRAM, address))
        return false;
    return connect(static_cast<SOCKET>(m_Handle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

int32_t Win64UdpSocket::SendTo(const void* InData, size_t InSize, const IpEndpoint& InTo) {
    if (!EnsureSocket())
        return -1;
    sockaddr_in address = {};
    if (!ResolveEndpoint(InTo, SOCK_DGRAM, address))
        return -1;
    return sendto(static_cast<SOCKET>(m_Handle), static_cast<const char*>(InData), static_cast<int>(InSize), 0, reinterpret_cast<sockaddr*>(&address), sizeof(address));
}

int32_t Win64UdpSocket::ReceiveFrom(void* OutData, size_t InSize, IpEndpoint& OutFrom) {
    if (m_Handle == -1)
        return -1;
    sockaddr_in address = {};
    int length = sizeof(address);
    int32_t received = recvfrom(static_cast<SOCKET>(m_Handle), static_cast<char*>(OutData), static_cast<int>(InSize), 0, reinterpret_cast<sockaddr*>(&address), &length);
    if (received >= 0)
        OutFrom = EndpointFromSockaddr(address);
    return received;
}

int32_t Win64UdpSocket::Send(const void* InData, size_t InSize) {
    if (m_Handle == -1)
        return -1;
    return send(static_cast<SOCKET>(m_Handle), static_cast<const char*>(InData), static_cast<int>(InSize), 0);
}

int32_t Win64UdpSocket::Receive(void* OutData, size_t InSize) {
    if (m_Handle == -1)
        return -1;
    return recv(static_cast<SOCKET>(m_Handle), static_cast<char*>(OutData), static_cast<int>(InSize), 0);
}

void Win64UdpSocket::SetBlocking(bool InBlocking) {
    if (EnsureSocket())
        SetSocketBlocking(static_cast<SOCKET>(m_Handle), InBlocking);
}

void Win64UdpSocket::SetBroadcastEnabled(bool InEnabled) {
    if (!EnsureSocket())
        return;
    BOOL value = InEnabled ? TRUE : FALSE;
    setsockopt(static_cast<SOCKET>(m_Handle), SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&value), sizeof(value));
}

uint16_t Win64UdpSocket::GetLocalPort() const {
    return m_Handle == -1 ? 0 : BoundPort(static_cast<SOCKET>(m_Handle));
}

bool Win64UdpSocket::IsValid() const { return m_Handle != -1; }

void Win64UdpSocket::Close() {
    if (m_Handle != -1) {
        closesocket(static_cast<SOCKET>(m_Handle));
        m_Handle = -1;
    }
}

Win64TcpSocket::~Win64TcpSocket() { Close(); }

bool Win64TcpSocket::EnsureSocket() {
    if (m_Handle != -1)
        return true;
    EnsureWinsock();
    SOCKET handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    m_Handle = handle == INVALID_SOCKET ? -1 : static_cast<int64_t>(handle);
    return m_Handle != -1;
}

bool Win64TcpSocket::Connect(const IpEndpoint& InPeer) {
    if (!EnsureSocket())
        return false;
    sockaddr_in address = {};
    if (!ResolveEndpoint(InPeer, SOCK_STREAM, address))
        return false;
    return connect(static_cast<SOCKET>(m_Handle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

bool Win64TcpSocket::Listen(uint16_t InPort, const String& InAddress, int32_t InBacklog) {
    if (!EnsureSocket())
        return false;
    BOOL reuse = TRUE;
    setsockopt(static_cast<SOCKET>(m_Handle), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    sockaddr_in address = {};
    if (!ResolveEndpoint({ InAddress, InPort }, SOCK_STREAM, address))
        return false;
    if (bind(static_cast<SOCKET>(m_Handle), reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        return false;
    return listen(static_cast<SOCKET>(m_Handle), InBacklog) == 0;
}

SharedObjectPtr<TcpSocket> Win64TcpSocket::Accept() {
    if (m_Handle == -1)
        return nullptr;
    SOCKET client = accept(static_cast<SOCKET>(m_Handle), nullptr, nullptr);
    if (client == INVALID_SOCKET)
        return nullptr;
    Win64TcpSocket* connection = new Win64TcpSocket();
    connection->m_Handle = static_cast<int64_t>(client);
    return SharedObjectPtr<TcpSocket>(connection);
}

int32_t Win64TcpSocket::Send(const void* InData, size_t InSize) {
    if (m_Handle == -1)
        return -1;
    return send(static_cast<SOCKET>(m_Handle), static_cast<const char*>(InData), static_cast<int>(InSize), 0);
}

int32_t Win64TcpSocket::Receive(void* OutData, size_t InSize) {
    if (m_Handle == -1)
        return -1;
    return recv(static_cast<SOCKET>(m_Handle), static_cast<char*>(OutData), static_cast<int>(InSize), 0);
}

void Win64TcpSocket::SetBlocking(bool InBlocking) {
    if (EnsureSocket())
        SetSocketBlocking(static_cast<SOCKET>(m_Handle), InBlocking);
}

void Win64TcpSocket::SetNoDelay(bool InEnabled) {
    if (!EnsureSocket())
        return;
    BOOL value = InEnabled ? TRUE : FALSE;
    setsockopt(static_cast<SOCKET>(m_Handle), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&value), sizeof(value));
}

IpEndpoint Win64TcpSocket::GetPeerEndpoint() const {
    if (m_Handle == -1)
        return {};
    sockaddr_in address = {};
    int length = sizeof(address);
    if (getpeername(static_cast<SOCKET>(m_Handle), reinterpret_cast<sockaddr*>(&address), &length) != 0)
        return {};
    return EndpointFromSockaddr(address);
}

uint16_t Win64TcpSocket::GetLocalPort() const {
    return m_Handle == -1 ? 0 : BoundPort(static_cast<SOCKET>(m_Handle));
}

bool Win64TcpSocket::IsValid() const { return m_Handle != -1; }

void Win64TcpSocket::Close() {
    if (m_Handle != -1) {
        closesocket(static_cast<SOCKET>(m_Handle));
        m_Handle = -1;
    }
}
