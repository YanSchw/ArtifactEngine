#include "Platform/PlatformHooks.h"

PlatformHooks& PlatformHooks::Get() {
    static PlatformHooks* hooks = [] {
        for (const Class& candidate : Class::GetSubclassesOf(StaticClass())) {
            if (candidate == StaticClass())
                continue;
            if (Object* instance = Object::Create(candidate))
                return Cast<PlatformHooks>(instance);
        }
        return new PlatformHooks();
    }();
    return *hooks;
}
