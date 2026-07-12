#include "LinuxTls.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace {
    // Drain ciphertext OpenSSL has queued in its write BIO out to the transport.
    bool FlushOutgoing(SSL* InSsl, TcpSocket* InTransport) {
        BIO* wbio = SSL_get_wbio(InSsl);
        char chunk[4096];
        int read = 0;
        while ((read = BIO_read(wbio, chunk, sizeof(chunk))) > 0) {
            if (!InTransport->SendAll(chunk, static_cast<size_t>(read)))
                return false;
        }
        return true;
    }

    // Pull one chunk of ciphertext from the transport into OpenSSL's read BIO.
    bool FeedIncoming(SSL* InSsl, TcpSocket* InTransport) {
        char chunk[4096];
        int32_t received = InTransport->Receive(chunk, sizeof(chunk));
        if (received <= 0)
            return false;
        BIO_write(SSL_get_rbio(InSsl), chunk, received);
        return true;
    }
}

LinuxTlsSocket::~LinuxTlsSocket() { Close(); }

bool LinuxTlsSocket::Connect(const IpEndpoint& InPeer, const String& InHostname) {
    m_Transport = TcpSocket::Create();
    if (!m_Transport || !m_Transport->Connect(InPeer))
        return false;

    SSL_CTX* context = SSL_CTX_new(TLS_client_method());
    if (!context)
        return false;
    SSL_CTX_set_verify(context, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_default_verify_paths(context);
    m_Context = context;

    SSL* ssl = SSL_new(context);
    if (!ssl) {
        Close();
        return false;
    }
    m_Ssl = ssl;

    SSL_set_tlsext_host_name(ssl, InHostname.c_str());
    SSL_set1_host(ssl, InHostname.c_str());
    SSL_set_bio(ssl, BIO_new(BIO_s_mem()), BIO_new(BIO_s_mem()));
    SSL_set_connect_state(ssl);

    while (true) {
        int result = SSL_do_handshake(ssl);
        if (!FlushOutgoing(ssl, m_Transport))
            break;
        if (result == 1)
            return true;
        int error = SSL_get_error(ssl, result);
        if (error == SSL_ERROR_WANT_READ) {
            if (!FeedIncoming(ssl, m_Transport))
                break;
        } else if (error != SSL_ERROR_WANT_WRITE) {
            break;
        }
    }

    AE_ERROR("TLS handshake with {0} failed", InHostname);
    Close();
    return false;
}

int32_t LinuxTlsSocket::Send(const void* InData, size_t InSize) {
    if (!m_Ssl)
        return -1;
    SSL* ssl = static_cast<SSL*>(m_Ssl);
    int written = SSL_write(ssl, InData, static_cast<int>(InSize));
    if (!FlushOutgoing(ssl, m_Transport))
        return -1;
    return written > 0 ? written : -1;
}

int32_t LinuxTlsSocket::Receive(void* OutData, size_t InSize) {
    if (!m_Ssl)
        return -1;
    SSL* ssl = static_cast<SSL*>(m_Ssl);
    while (true) {
        int read = SSL_read(ssl, OutData, static_cast<int>(InSize));
        if (read > 0)
            return read;
        int error = SSL_get_error(ssl, read);
        if (error == SSL_ERROR_WANT_READ) {
            if (!FlushOutgoing(ssl, m_Transport) || !FeedIncoming(ssl, m_Transport))
                return 0;
        } else if (error == SSL_ERROR_ZERO_RETURN) {
            return 0;
        } else {
            return -1;
        }
    }
}

bool LinuxTlsSocket::IsValid() const {
    return m_Ssl != nullptr && m_Transport && m_Transport->IsValid();
}

void LinuxTlsSocket::Close() {
    if (m_Ssl) {
        SSL_shutdown(static_cast<SSL*>(m_Ssl));
        SSL_free(static_cast<SSL*>(m_Ssl));
        m_Ssl = nullptr;
    }
    if (m_Context) {
        SSL_CTX_free(static_cast<SSL_CTX*>(m_Context));
        m_Context = nullptr;
    }
    if (m_Transport) {
        m_Transport->Close();
        m_Transport = nullptr;
    }
}
