#include "Version.h"

#include <format>

extern int __VERSION_MAJOR();
extern int __VERSION_MINOR();
extern int __VERSION_PATCH();

int32_t Version::Major() {
    return __VERSION_MAJOR();
}

int32_t Version::Minor() {
    return __VERSION_MINOR();
}

int32_t Version::Patch() {
    return __VERSION_PATCH();
}

String Version::GetVersionString() {
    if (Patch() >= 0) {
        return std::format("{0}.{1}.{2}", Major(), Minor(), Patch());
    }
    return std::format("{0}.{1}", Major(), Minor());
}
