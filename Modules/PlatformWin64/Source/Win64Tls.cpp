#include "Win64Tls.h"

#define SECURITY_WIN32
#include <winsock2.h>
#include <windows.h>
#include <security.h>
#include <schannel.h>
#include <string>

#undef ERROR // windows.h/wingdi.h defines ERROR as 0, which clashes with LogLevel::ERROR

#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

struct SChannelState {
    CredHandle Credentials = {};
    CtxtHandle Context = {};
    SecPkgContext_StreamSizes Sizes = {};
    bool HasCredentials = false;
    bool HasContext = false;
    std::string IncomingCipher;   // ciphertext read from the transport, not yet decrypted
    std::string DecryptedPlain;   // decrypted application bytes not yet handed to the caller
};

namespace {
    bool AcquireCredentials(SChannelState* InState) {
        SCHANNEL_CRED cred = {};
        cred.dwVersion = SCHANNEL_CRED_VERSION;
        cred.dwFlags = SCH_CRED_AUTO_CRED_VALIDATION | SCH_CRED_NO_DEFAULT_CREDS | SCH_USE_STRONG_CRYPTO;

        SECURITY_STATUS status = AcquireCredentialsHandleW(
            nullptr, const_cast<SEC_WCHAR*>(UNISP_NAME_W), SECPKG_CRED_OUTBOUND,
            nullptr, &cred, nullptr, nullptr, &InState->Credentials, nullptr);

        InState->HasCredentials = status == SEC_E_OK;
        return InState->HasCredentials;
    }

    bool PerformHandshake(SChannelState* InState, TcpSocket* InTransport, const String& InHostname) {
        std::wstring target(InHostname.begin(), InHostname.end());
        const DWORD requestFlags = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY
            | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM;

        std::string incoming;
        bool first = true;
        bool readMore = false;

        while (true) {
            if (readMore) {
                char chunk[8192];
                int32_t received = InTransport->Receive(chunk, sizeof(chunk));
                if (received <= 0)
                    return false;
                incoming.append(chunk, static_cast<size_t>(received));
                readMore = false;
            }

            SecBuffer inBuffers[2] = {};
            inBuffers[0].BufferType = SECBUFFER_TOKEN;
            inBuffers[0].pvBuffer = incoming.data();
            inBuffers[0].cbBuffer = static_cast<unsigned long>(incoming.size());
            inBuffers[1].BufferType = SECBUFFER_EMPTY;
            SecBufferDesc inDesc = { SECBUFFER_VERSION, 2, inBuffers };

            SecBuffer outBuffers[1] = {};
            outBuffers[0].BufferType = SECBUFFER_TOKEN;
            SecBufferDesc outDesc = { SECBUFFER_VERSION, 1, outBuffers };

            DWORD outFlags = 0;
            SECURITY_STATUS status = InitializeSecurityContextW(
                &InState->Credentials,
                first ? nullptr : &InState->Context,
                target.data(),
                requestFlags, 0, 0,
                first ? nullptr : &inDesc,
                0,
                first ? &InState->Context : nullptr,
                &outDesc, &outFlags, nullptr);
            first = false;

            if (outBuffers[0].cbBuffer > 0 && outBuffers[0].pvBuffer) {
                bool ok = InTransport->SendAll(outBuffers[0].pvBuffer, outBuffers[0].cbBuffer);
                FreeContextBuffer(outBuffers[0].pvBuffer);
                if (!ok)
                    return false;
            }

            if (status == SEC_E_INCOMPLETE_MESSAGE) {
                readMore = true;
                continue;
            }

            // Ciphertext beyond the consumed handshake record is carried into the next step.
            std::string extra;
            if (inBuffers[1].BufferType == SECBUFFER_EXTRA)
                extra.assign(incoming.end() - inBuffers[1].cbBuffer, incoming.end());

            if (status == SEC_E_OK) {
                InState->HasContext = true;
                InState->IncomingCipher = extra;
                return QueryContextAttributesW(&InState->Context, SECPKG_ATTR_STREAM_SIZES, &InState->Sizes) == SEC_E_OK;
            }

            if (status == SEC_I_CONTINUE_NEEDED) {
                incoming = extra;
                readMore = extra.empty();
                continue;
            }

            return false;
        }
    }
}

Win64TlsSocket::~Win64TlsSocket() { Close(); }

bool Win64TlsSocket::Connect(const IpEndpoint& InPeer, const String& InHostname) {
    m_Transport = TcpSocket::Create();
    if (!m_Transport || !m_Transport->Connect(InPeer))
        return false;

    m_State = new SChannelState();
    if (!AcquireCredentials(m_State) || !PerformHandshake(m_State, m_Transport, InHostname)) {
        AE_ERROR("TLS handshake with {0} failed", InHostname);
        Close();
        return false;
    }
    return true;
}

int32_t Win64TlsSocket::Send(const void* InData, size_t InSize) {
    if (!m_State || !m_State->HasContext)
        return -1;

    const byte* cursor = static_cast<const byte*>(InData);
    size_t remaining = InSize;
    std::string record;
    while (remaining > 0) {
        unsigned long chunk = static_cast<unsigned long>(remaining < m_State->Sizes.cbMaximumMessage ? remaining : m_State->Sizes.cbMaximumMessage);
        record.resize(m_State->Sizes.cbHeader + chunk + m_State->Sizes.cbTrailer);
        memcpy(record.data() + m_State->Sizes.cbHeader, cursor, chunk);

        SecBuffer buffers[4] = {};
        buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
        buffers[0].pvBuffer = record.data();
        buffers[0].cbBuffer = m_State->Sizes.cbHeader;
        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = record.data() + m_State->Sizes.cbHeader;
        buffers[1].cbBuffer = chunk;
        buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
        buffers[2].pvBuffer = record.data() + m_State->Sizes.cbHeader + chunk;
        buffers[2].cbBuffer = m_State->Sizes.cbTrailer;
        buffers[3].BufferType = SECBUFFER_EMPTY;
        SecBufferDesc desc = { SECBUFFER_VERSION, 4, buffers };

        if (EncryptMessage(&m_State->Context, 0, &desc, 0) != SEC_E_OK)
            return -1;

        unsigned long total = buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer;
        if (!m_Transport->SendAll(record.data(), total))
            return -1;

        cursor += chunk;
        remaining -= chunk;
    }
    return static_cast<int32_t>(InSize);
}

int32_t Win64TlsSocket::Receive(void* OutData, size_t InSize) {
    if (!m_State || !m_State->HasContext)
        return -1;

    while (m_State->DecryptedPlain.empty()) {
        if (m_State->IncomingCipher.empty()) {
            char chunk[8192];
            int32_t received = m_Transport->Receive(chunk, sizeof(chunk));
            if (received <= 0)
                return received == 0 ? 0 : -1;
            m_State->IncomingCipher.append(chunk, static_cast<size_t>(received));
        }

        SecBuffer buffers[4] = {};
        buffers[0].BufferType = SECBUFFER_DATA;
        buffers[0].pvBuffer = m_State->IncomingCipher.data();
        buffers[0].cbBuffer = static_cast<unsigned long>(m_State->IncomingCipher.size());
        buffers[1].BufferType = SECBUFFER_EMPTY;
        buffers[2].BufferType = SECBUFFER_EMPTY;
        buffers[3].BufferType = SECBUFFER_EMPTY;
        SecBufferDesc desc = { SECBUFFER_VERSION, 4, buffers };

        SECURITY_STATUS status = DecryptMessage(&m_State->Context, &desc, 0, nullptr);
        if (status == SEC_E_INCOMPLETE_MESSAGE) {
            char chunk[8192];
            int32_t received = m_Transport->Receive(chunk, sizeof(chunk));
            if (received <= 0)
                return received == 0 ? 0 : -1;
            m_State->IncomingCipher.append(chunk, static_cast<size_t>(received));
            continue;
        }
        if (status == SEC_I_CONTEXT_EXPIRED)
            return 0;
        if (status != SEC_E_OK && status != SEC_I_RENEGOTIATE)
            return -1;

        std::string extra;
        for (const SecBuffer& buffer : buffers) {
            if (buffer.BufferType == SECBUFFER_DATA && buffer.cbBuffer > 0)
                m_State->DecryptedPlain.append(static_cast<const char*>(buffer.pvBuffer), buffer.cbBuffer);
            else if (buffer.BufferType == SECBUFFER_EXTRA && buffer.cbBuffer > 0)
                extra.assign(static_cast<const char*>(buffer.pvBuffer), buffer.cbBuffer);
        }
        m_State->IncomingCipher = extra;
    }

    size_t count = m_State->DecryptedPlain.size() < InSize ? m_State->DecryptedPlain.size() : InSize;
    memcpy(OutData, m_State->DecryptedPlain.data(), count);
    m_State->DecryptedPlain.erase(0, count);
    return static_cast<int32_t>(count);
}

bool Win64TlsSocket::IsValid() const {
    return m_State != nullptr && m_State->HasContext && m_Transport && m_Transport->IsValid();
}

void Win64TlsSocket::Close() {
    if (m_State) {
        if (m_State->HasContext)
            DeleteSecurityContext(&m_State->Context);
        if (m_State->HasCredentials)
            FreeCredentialsHandle(&m_State->Credentials);
        delete m_State;
        m_State = nullptr;
    }
    if (m_Transport) {
        m_Transport->Close();
        m_Transport = nullptr;
    }
}
