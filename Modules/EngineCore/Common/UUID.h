#pragma once
#include "Common/String.h"
#include "Object/Struct.h"
#include "UUID.gen.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <functional>

struct UUID {
    ARTIFACT_STRUCT();

    using Storage = std::array<uint8_t, 16>;

    Storage Data{};

    constexpr UUID() = default;
    constexpr explicit UUID(const Storage& data)
        : Data(data) {}

    // Generate a random RFC 4122 version 4 UUID
    static UUID Generate();

    // Parse UUID from:
    // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
    //
    // Returns false on invalid format.
    static bool TryParse(const String& str, UUID& out);

    // Convenience wrapper.
    // Returns empty UUID on failure.
    static UUID FromString(const String& str);

    // Convert to canonical string form
    String ToString() const;

    bool IsValid() const {
        return Data != Storage{};
    }

    bool operator==(const UUID& other) const {
        return Data == other.Data;
    }

    bool operator!=(const UUID& other) const {
        return !(*this == other);
    }

    bool operator<(const UUID& other) const {
        return Data < other.Data;
    }

    const uint8_t* Bytes() const {
        return Data.data();
    }

    uint8_t* Bytes() {
        return Data.data();
    }
};

namespace std {

template<>
struct hash<UUID> {
    size_t operator()(const UUID& u) const noexcept {
        uint64_t a = 0;
        uint64_t b = 0;

        std::memcpy(&a, u.Data.data(), 8);
        std::memcpy(&b, u.Data.data() + 8, 8);

        size_t h1 = std::hash<uint64_t>{}(a);
        size_t h2 = std::hash<uint64_t>{}(b);

        return h1 ^ (h2 << 1);
    }
};

}