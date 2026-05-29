#include "UUID.h"

#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>

namespace {

bool HexValue(char c, uint8_t& out) {
    if (c >= '0' && c <= '9') {
        out = static_cast<uint8_t>(c - '0');
        return true;
    }

    if (c >= 'a' && c <= 'f') {
        out = static_cast<uint8_t>(10 + (c - 'a'));
        return true;
    }

    if (c >= 'A' && c <= 'F') {
        out = static_cast<uint8_t>(10 + (c - 'A'));
        return true;
    }

    return false;
}

}

UUID UUID::Generate() {
    Storage s{};

    // Thread-local generator avoids rebuilding RNG every call
    thread_local std::mt19937_64 gen(std::random_device{}());

    for (size_t i = 0; i < s.size(); i += 8) {
        uint64_t value = gen();

        for (size_t j = 0; j < 8 && (i + j) < s.size(); ++j) {
            s[i + j] = static_cast<uint8_t>(
                (value >> (j * 8)) & 0xFF
            );
        }
    }

    // RFC 4122 version 4
    s[6] = (s[6] & 0x0F) | 0x40;

    // RFC 4122 variant
    s[8] = (s[8] & 0x3F) | 0x80;

    return UUID(s);
}

bool UUID::TryParse(const String& str, UUID& out) {
    String filtered;
    filtered.reserve(32);

    for (char c : str) {
        if (c != '-') {
            filtered.push_back(c);
        }
    }

    if (filtered.size() != 32) {
        return false;
    }

    Storage s{};

    for (size_t i = 0; i < 16; ++i) {
        uint8_t hi = 0;
        uint8_t lo = 0;

        if (!HexValue(filtered[i * 2], hi)) {
            return false;
        }

        if (!HexValue(filtered[i * 2 + 1], lo)) {
            return false;
        }

        s[i] = static_cast<uint8_t>((hi << 4) | lo);
    }

    out = UUID(s);
    return true;
}

UUID UUID::FromString(const String& str) {
    UUID result;

    if (!TryParse(str, result)) {
        return UUID{};
    }

    return result;
}

String UUID::ToString() const {
    std::ostringstream oss;

    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < Data.size(); ++i) {
        oss << std::setw(2)
            << static_cast<int>(Data[i]);

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            oss << '-';
        }
    }

    return oss.str();
}