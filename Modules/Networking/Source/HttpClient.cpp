#include "HttpClient.h"

#if defined(AE_PLATFORM_WEB)
#include <emscripten/emscripten.h>
#else
#include "TcpSocket.h"
#include "TlsSocket.h"
#endif

namespace {
    String Trim(const String& InText) {
        size_t begin = InText.find_first_not_of(" \t\r\n");
        if (begin == String::npos)
            return "";
        size_t end = InText.find_last_not_of(" \t\r\n");
        return InText.substr(begin, end - begin + 1);
    }

    HttpResponse ParseResponse(const String& InRaw) {
        HttpResponse response;

        size_t headerEnd = InRaw.find("\r\n\r\n");
        String head = headerEnd == String::npos ? InRaw : InRaw.substr(0, headerEnd);
        response.Body = headerEnd == String::npos ? "" : InRaw.substr(headerEnd + 4);

        size_t lineEnd = head.find("\r\n");
        String statusLine = lineEnd == String::npos ? head : head.substr(0, lineEnd);

        size_t firstSpace = statusLine.find(' ');
        if (firstSpace != String::npos) {
            size_t secondSpace = statusLine.find(' ', firstSpace + 1);
            String code = statusLine.substr(firstSpace + 1, secondSpace == String::npos ? String::npos : secondSpace - firstSpace - 1);
            response.StatusCode = std::atoi(code.c_str());
            if (secondSpace != String::npos)
                response.StatusMessage = statusLine.substr(secondSpace + 1);
        }

        size_t cursor = lineEnd == String::npos ? head.size() : lineEnd + 2;
        while (cursor < head.size()) {
            size_t next = head.find("\r\n", cursor);
            String line = head.substr(cursor, next == String::npos ? String::npos : next - cursor);
            size_t colon = line.find(':');
            if (colon != String::npos)
                response.Headers[Trim(line.substr(0, colon))] = Trim(line.substr(colon + 1));
            if (next == String::npos)
                break;
            cursor = next + 2;
        }

        return response;
    }

#if !defined(AE_PLATFORM_WEB)
    struct ParsedUrl {
        String Host;
        uint16_t Port = 80;
        String Path = "/";
        bool Secure = false;
        bool Valid = false;
    };

    ParsedUrl ParseUrl(const String& InUrl) {
        ParsedUrl result;
        String rest = InUrl;

        size_t scheme = rest.find("://");
        if (scheme != String::npos) {
            String protocol = rest.substr(0, scheme);
            if (protocol == "https") {
                result.Secure = true;
                result.Port = 443;
            } else if (protocol != "http") {
                AE_ERROR("HttpClient only supports http/https URLs, got '{0}'", protocol);
                return result;
            }
            rest = rest.substr(scheme + 3);
        }

        size_t pathStart = rest.find('/');
        String authority = pathStart == String::npos ? rest : rest.substr(0, pathStart);
        result.Path = pathStart == String::npos ? "/" : rest.substr(pathStart);

        size_t portSep = authority.find(':');
        if (portSep == String::npos) {
            result.Host = authority;
        } else {
            result.Host = authority.substr(0, portSep);
            result.Port = static_cast<uint16_t>(std::stoi(authority.substr(portSep + 1)));
        }

        result.Valid = !result.Host.empty();
        return result;
    }

    SharedObjectPtr<NetworkStream> OpenStream(const ParsedUrl& InUrl) {
        if (InUrl.Secure) {
            SharedObjectPtr<TlsSocket> tls = TlsSocket::Create();
            if (tls && tls->Connect({ InUrl.Host, InUrl.Port }, InUrl.Host))
                return tls;
            return nullptr;
        }

        SharedObjectPtr<TcpSocket> tcp = TcpSocket::Create();
        if (tcp && tcp->Connect({ InUrl.Host, InUrl.Port }))
            return tcp;
        return nullptr;
    }
#else
    String SerializeHeaders(const String& InContentType, const Map<String, String>& InHeaders) {
        String headers;
        if (!InContentType.empty())
            headers += "Content-Type: " + InContentType + "\r\n";
        for (const auto& [key, value] : InHeaders)
            headers += key + ": " + value + "\r\n";
        return headers;
    }
#endif
}

#if defined(AE_PLATFORM_WEB)

EM_JS(int, ArtifactWebHttpSend, (const char* InMethod, const char* InUrl, const char* InBody, int InBodySize, const char* InHeaders), {
    globalThis.__ArtifactHttpResponse = null;
    var request = new XMLHttpRequest();
    try {
        request.open(UTF8ToString(InMethod), UTF8ToString(InUrl), false);
        request.overrideMimeType('text/plain; charset=x-user-defined');
        for (var header of UTF8ToString(InHeaders).split('\r\n')) {
            var separator = header.indexOf(': ');
            if (separator > 0) request.setRequestHeader(header.slice(0, separator), header.slice(separator + 2));
        }
        request.send(InBodySize ? HEAPU8.slice(InBody, InBody + InBodySize) : null);
    } catch (error) {
        return -1;
    }
    globalThis.__ArtifactHttpResponse = 'HTTP/1.1 ' + request.status + ' ' + request.statusText + '\r\n'
        + request.getAllResponseHeaders() + '\r\n' + request.responseText;
    return globalThis.__ArtifactHttpResponse.length;
});

EM_JS(void, ArtifactWebHttpRead, (char* OutData, int InSize), {
    var raw = globalThis.__ArtifactHttpResponse;
    for (var i = 0; i < InSize; ++i) HEAPU8[OutData + i] = raw.charCodeAt(i) & 255;
    globalThis.__ArtifactHttpResponse = null;
});

#endif

HttpResponse HttpClient::Get(const String& InUrl, const Map<String, String>& InHeaders) {
    return Request("GET", InUrl, "", "", InHeaders);
}

HttpResponse HttpClient::Post(const String& InUrl, const String& InBody, const String& InContentType, const Map<String, String>& InHeaders) {
    return Request("POST", InUrl, InBody, InContentType, InHeaders);
}

HttpResponse HttpClient::Put(const String& InUrl, const String& InBody, const String& InContentType, const Map<String, String>& InHeaders) {
    return Request("PUT", InUrl, InBody, InContentType, InHeaders);
}

HttpResponse HttpClient::Delete(const String& InUrl, const Map<String, String>& InHeaders) {
    return Request("DELETE", InUrl, "", "", InHeaders);
}

HttpResponse HttpClient::Head(const String& InUrl, const Map<String, String>& InHeaders) {
    return Request("HEAD", InUrl, "", "", InHeaders);
}

HttpResponse HttpClient::Query(const String& InUrl, const String& InBody, const String& InContentType, const Map<String, String>& InHeaders) {
    return Request("QUERY", InUrl, InBody, InContentType, InHeaders);
}

#if defined(AE_PLATFORM_WEB)

HttpResponse HttpClient::Request(const String& InMethod, const String& InUrl, const String& InBody, const String& InContentType, const Map<String, String>& InHeaders) {
    const String headers = SerializeHeaders(InContentType, InHeaders);
    const int32_t size = ArtifactWebHttpSend(InMethod.c_str(), InUrl.c_str(), InBody.data(), static_cast<int32_t>(InBody.size()), headers.c_str());
    if (size < 0) {
        AE_ERROR("HttpClient failed to request {0}", InUrl);
        return HttpResponse();
    }

    String raw(static_cast<size_t>(size), '\0');
    ArtifactWebHttpRead(raw.data(), size);
    return ParseResponse(raw);
}

#else

HttpResponse HttpClient::Request(const String& InMethod, const String& InUrl, const String& InBody, const String& InContentType, const Map<String, String>& InHeaders) {
    HttpResponse response;

    ParsedUrl url = ParseUrl(InUrl);
    if (!url.Valid)
        return response;

    SharedObjectPtr<NetworkStream> socket = OpenStream(url);
    if (!socket) {
        AE_ERROR("HttpClient failed to connect to {0}:{1}", url.Host, url.Port);
        return response;
    }

    String request = InMethod + " " + url.Path + " HTTP/1.1\r\n";
    request += "Host: " + url.Host + "\r\n";
    request += "Connection: close\r\n";
    if (!InContentType.empty())
        request += "Content-Type: " + InContentType + "\r\n";
    if (!InBody.empty())
        request += "Content-Length: " + std::to_string(InBody.size()) + "\r\n";
    for (const auto& [key, value] : InHeaders)
        request += key + ": " + value + "\r\n";
    request += "\r\n";
    request += InBody;

    if (!socket->SendAll(request)) {
        AE_ERROR("HttpClient failed to send request to {0}", url.Host);
        return response;
    }

    String raw;
    char buffer[4096];
    while (true) {
        int32_t received = socket->Receive(buffer, sizeof(buffer));
        if (received <= 0)
            break;
        raw.append(buffer, static_cast<size_t>(received));
    }

    return ParseResponse(raw);
}

#endif
