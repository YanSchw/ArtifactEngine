#include "MacOSTls.h"

#include <Security/SecureTransport.h>

// Secure Transport is deprecated in favour of Network.framework, but remains the only API
// that layers TLS onto an already-connected socket we own. It is fully functional.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace {
    OSStatus TransportRead(SSLConnectionRef InConnection, void* OutData, size_t* InOutLength) {
        TcpSocket* transport = static_cast<TcpSocket*>(const_cast<void*>(InConnection));
        size_t requested = *InOutLength;
        size_t total = 0;
        while (total < requested) {
            int32_t received = transport->Receive(static_cast<byte*>(OutData) + total, requested - total);
            if (received > 0) {
                total += static_cast<size_t>(received);
            } else {
                *InOutLength = total;
                return received == 0 ? errSSLClosedGraceful : errSSLClosedAbort;
            }
        }
        *InOutLength = total;
        return noErr;
    }

    OSStatus TransportWrite(SSLConnectionRef InConnection, const void* InData, size_t* InOutLength) {
        TcpSocket* transport = static_cast<TcpSocket*>(const_cast<void*>(InConnection));
        return transport->SendAll(InData, *InOutLength) ? noErr : errSSLClosedAbort;
    }
}

MacOSTlsSocket::~MacOSTlsSocket() { Close(); }

bool MacOSTlsSocket::Connect(const IpEndpoint& InPeer, const String& InHostname) {
    m_Transport = TcpSocket::Create();
    if (!m_Transport || !m_Transport->Connect(InPeer))
        return false;

    SSLContextRef context = SSLCreateContext(kCFAllocatorDefault, kSSLClientSide, kSSLStreamType);
    if (!context)
        return false;
    m_Context = context;

    if (SSLSetIOFuncs(context, TransportRead, TransportWrite) != noErr
        || SSLSetConnection(context, m_Transport.Get()) != noErr
        || SSLSetPeerDomainName(context, InHostname.c_str(), InHostname.size()) != noErr) {
        Close();
        return false;
    }

    OSStatus status;
    do {
        status = SSLHandshake(context);
    } while (status == errSSLWouldBlock);

    if (status != noErr) {
        AE_ERROR("TLS handshake with {0} failed ({1})", InHostname, static_cast<int32_t>(status));
        Close();
        return false;
    }
    return true;
}

int32_t MacOSTlsSocket::Send(const void* InData, size_t InSize) {
    if (!m_Context)
        return -1;
    size_t processed = 0;
    OSStatus status = SSLWrite(static_cast<SSLContextRef>(m_Context), InData, InSize, &processed);
    if (status != noErr && status != errSSLWouldBlock)
        return -1;
    return static_cast<int32_t>(processed);
}

int32_t MacOSTlsSocket::Receive(void* OutData, size_t InSize) {
    if (!m_Context)
        return -1;
    size_t processed = 0;
    OSStatus status = SSLRead(static_cast<SSLContextRef>(m_Context), OutData, InSize, &processed);
    if (status == errSSLClosedGraceful || status == errSSLClosedNoNotify)
        return static_cast<int32_t>(processed);
    if (status != noErr && status != errSSLWouldBlock)
        return processed > 0 ? static_cast<int32_t>(processed) : -1;
    return static_cast<int32_t>(processed);
}

bool MacOSTlsSocket::IsValid() const {
    return m_Context != nullptr && m_Transport && m_Transport->IsValid();
}

void MacOSTlsSocket::Close() {
    if (m_Context) {
        SSLClose(static_cast<SSLContextRef>(m_Context));
        CFRelease(static_cast<SSLContextRef>(m_Context));
        m_Context = nullptr;
    }
    if (m_Transport) {
        m_Transport->Close();
        m_Transport = nullptr;
    }
}

#pragma clang diagnostic pop
