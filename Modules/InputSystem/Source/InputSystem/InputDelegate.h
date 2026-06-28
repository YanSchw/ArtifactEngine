#pragma once
#include "Object/Pointer.h"
#include <functional>

// A multicast callback list whose listeners live only as long as their owning
// Object. Each listener holds a WeakObjectPtr to its owner; when that owner is
// destroyed the weak ref nullifies and the listener is pruned on the next
// Broadcast.
template<typename... Args>
class InputDelegate {
public:
    using CallbackFn = std::function<void(Args...)>;

    void Bind(Object* InOwner, CallbackFn InCallback) {
        AE_ASSERT(InOwner, "InputDelegate requires a non-null owner for lifetime tracking");
        m_Listeners.Add({WeakObjectPtr<Object>(InOwner), std::move(InCallback)});
    }

    void Unbind(Object* InOwner) {
        for (int32_t i = m_Listeners.Last(); i >= 0; i--) {
            if (m_Listeners[i].Owner.Get() == InOwner) {
                m_Listeners.RemoveAt(i);
            }
        }
    }

    void Broadcast(Args... InArgs) {
        for (int32_t i = m_Listeners.Last(); i >= 0; i--) {
            Listener& listener = m_Listeners[i];
            if (!listener.Owner) {  // owner destroyed -> prune
                m_Listeners.RemoveAt(i);
                continue;
            }
            listener.Function(InArgs...);
        }
    }

    bool HasListeners() const {
        return m_Listeners.Size() > 0;
    }

private:
    struct Listener {
        WeakObjectPtr<Object> Owner;
        CallbackFn Function;
    };
    Array<Listener> m_Listeners;
};
