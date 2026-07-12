#pragma once
#include "CoreMinimal.h"

struct HttpResponse {
    int32_t StatusCode = 0;
    String StatusMessage;
    Map<String, String> Headers;
    String Body;

    bool IsSuccess() const { return StatusCode >= 200 && StatusCode < 300; }
};

/* Blocking HTTP/1.1 client. Speaks plain http:// over TcpSocket and https:// over TlsSocket. */
class HttpClient {
public:
    HttpResponse Get(const String& InUrl, const Map<String, String>& InHeaders = {});
    HttpResponse Post(const String& InUrl, const String& InBody, const String& InContentType = "application/json", const Map<String, String>& InHeaders = {});
    HttpResponse Put(const String& InUrl, const String& InBody, const String& InContentType = "application/json", const Map<String, String>& InHeaders = {});
    HttpResponse Delete(const String& InUrl, const Map<String, String>& InHeaders = {});
    HttpResponse Head(const String& InUrl, const Map<String, String>& InHeaders = {});
    HttpResponse Query(const String& InUrl, const String& InBody, const String& InContentType = "application/json", const Map<String, String>& InHeaders = {});

    HttpResponse Request(const String& InMethod, const String& InUrl, const String& InBody = "", const String& InContentType = "", const Map<String, String>& InHeaders = {});
};
