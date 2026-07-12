#include "TcpSocket.h"

SharedObjectPtr<TcpSocket> TcpSocket::Create() {
    for (const Class& candidate : Class::GetSubclassesOf(StaticClass())) {
        if (candidate == StaticClass())
            continue;
        if (Object* instance = Object::Create(candidate))
            return SharedObjectPtr<TcpSocket>(Cast<TcpSocket>(instance));
    }
    AE_ERROR("No platform TcpSocket implementation is registered");
    return nullptr;
}
