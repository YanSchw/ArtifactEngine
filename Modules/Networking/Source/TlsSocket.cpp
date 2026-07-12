#include "TlsSocket.h"

SharedObjectPtr<TlsSocket> TlsSocket::Create() {
    for (const Class& candidate : Class::GetSubclassesOf(StaticClass())) {
        if (candidate == StaticClass())
            continue;
        if (Object* instance = Object::Create(candidate))
            return SharedObjectPtr<TlsSocket>(Cast<TlsSocket>(instance));
    }
    AE_ERROR("No platform TlsSocket implementation is registered");
    return nullptr;
}
