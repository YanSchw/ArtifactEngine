#pragma once
#include "Common/Array.h"
#include "Object/Pointer.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

struct DelegateHandle {
    uint64_t Id = 0;

    bool IsValid() const { return Id != 0; }
    bool operator==(const DelegateHandle& InOther) const = default;
};

template<typename... Args>
class Delegate {
public:
    using FunctionType = std::function<void(Args...)>;

    Delegate() = default;
    Delegate(const Delegate& InOther) { CopyBindings(InOther); }
    Delegate(Delegate&& InOther) noexcept = default;
    Delegate& operator=(Delegate&& InOther) noexcept = default;

    Delegate& operator=(const Delegate& InOther) {
        if (this != &InOther) {
            Clear();
            CopyBindings(InOther);
        }
        return *this;
    }

    DelegateHandle Add(FunctionType InFunction) {
        return Bind(nullptr, std::move(InFunction));
    }

    DelegateHandle Add(Object* InOwner, FunctionType InFunction) {
        return Bind(InOwner, std::move(InFunction));
    }

    template<typename T, typename U> requires std::is_base_of_v<U, T>
    DelegateHandle Add(T* InObject, void (U::*InMethod)(Args...)) {
        return Bind(OwnerOf(InObject), [InObject, InMethod](Args... InArgs) { (InObject->*InMethod)(InArgs...); });
    }

    template<typename T, typename U> requires std::is_base_of_v<U, T>
    DelegateHandle Add(const T* InObject, void (U::*InMethod)(Args...) const) {
        return Bind(OwnerOf(InObject), [InObject, InMethod](Args... InArgs) { (InObject->*InMethod)(InArgs...); });
    }

    bool Remove(DelegateHandle InHandle) {
        bool removed = false;
        for (const std::shared_ptr<Binding>& binding : m_Bindings) {
            if (InHandle.IsValid() && binding->Handle == InHandle) {
                binding->Handle = DelegateHandle();
                removed = true;
            }
        }
        Compact();
        return removed;
    }

    void RemoveAll(const Object* InOwner) {
        for (const std::shared_ptr<Binding>& binding : m_Bindings) {
            if (binding->HasOwner && binding->Owner.Get() == InOwner) {
                binding->Handle = DelegateHandle();
            }
        }
        Compact();
    }

    void Clear() {
        for (const std::shared_ptr<Binding>& binding : m_Bindings) {
            binding->Handle = DelegateHandle();
        }
        Compact();
    }

    bool IsBound() const { return GetBindingCount() > 0; }

    int32_t GetBindingCount() const {
        int32_t count = 0;
        for (const std::shared_ptr<Binding>& binding : m_Bindings) {
            count += binding->IsAlive() ? 1 : 0;
        }
        return count;
    }

    void Broadcast(Args... InArgs) const {
        m_Broadcasting++;
        const int32_t count = m_Bindings.Size();
        for (int32_t i = 0; i < count; i++) {
            const std::shared_ptr<Binding> binding = m_Bindings[i];
            if (binding->IsAlive()) {
                binding->Callback(InArgs...);
            }
        }
        m_Broadcasting--;
        Compact();
    }

    void operator()(Args... InArgs) const { Broadcast(InArgs...); }

private:
    struct Binding {
        DelegateHandle Handle;
        WeakObjectPtr<Object> Owner;
        bool HasOwner = false;
        FunctionType Callback;

        bool IsAlive() const { return Handle.IsValid() && (!HasOwner || Owner.Get()); }
    };

    template<typename T>
    static Object* OwnerOf(const T* InObject) {
        if constexpr (std::is_base_of_v<Object, T>) {
            return const_cast<T*>(InObject);
        } else {
            return nullptr;
        }
    }

    DelegateHandle Bind(Object* InOwner, FunctionType InFunction) {
        std::shared_ptr<Binding> binding = std::make_shared<Binding>();
        binding->Handle = DelegateHandle{ ++m_NextId };
        binding->Owner = InOwner;
        binding->HasOwner = InOwner != nullptr;
        binding->Callback = std::move(InFunction);
        m_Bindings.Add(binding);
        return binding->Handle;
    }

    void CopyBindings(const Delegate& InOther) {
        for (const std::shared_ptr<Binding>& binding : InOther.m_Bindings) {
            if (binding->IsAlive()) {
                Bind(binding->Owner.Get(), binding->Callback);
            }
        }
    }

    void Compact() const {
        if (m_Broadcasting == 0) {
            m_Bindings.Filter([](const std::shared_ptr<Binding>& InBinding) { return InBinding->IsAlive(); });
        }
    }

    mutable Array<std::shared_ptr<Binding>> m_Bindings;
    mutable int32_t m_Broadcasting = 0;
    uint64_t m_NextId = 0;
};
