#include "UdpSocket.h"

SharedObjectPtr<UdpSocket> UdpSocket::Create() {
    for (const Class& candidate : Class::GetSubclassesOf(StaticClass())) {
        if (candidate == StaticClass())
            continue;
        if (Object* instance = Object::Create(candidate))
            return SharedObjectPtr<UdpSocket>(Cast<UdpSocket>(instance));
    }
    AE_ERROR("No platform UdpSocket implementation is registered");
    return nullptr;
}
