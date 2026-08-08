#include "Color.h"

#include <cmath>
#include <format>

namespace {

int32_t HexDigit(char InCharacter) {
    if (InCharacter >= '0' && InCharacter <= '9') return InCharacter - '0';
    if (InCharacter >= 'a' && InCharacter <= 'f') return InCharacter - 'a' + 10;
    if (InCharacter >= 'A' && InCharacter <= 'F') return InCharacter - 'A' + 10;
    return -1;
}

}

Vec3 ColorUtils::RgbToHsv(const Vec3& InRgb) {
    const float high = glm::max(InRgb.r, glm::max(InRgb.g, InRgb.b));
    const float low = glm::min(InRgb.r, glm::min(InRgb.g, InRgb.b));
    const float range = high - low;

    float hue = 0.0f;
    if (range > 0.0f) {
        if (high == InRgb.r) {
            hue = 60.0f * std::fmod((InRgb.g - InRgb.b) / range, 6.0f);
        } else if (high == InRgb.g) {
            hue = 60.0f * ((InRgb.b - InRgb.r) / range + 2.0f);
        } else {
            hue = 60.0f * ((InRgb.r - InRgb.g) / range + 4.0f);
        }
    }
    if (hue < 0.0f) {
        hue += 360.0f;
    }
    return Vec3(hue, high > 0.0f ? range / high : 0.0f, high);
}

Vec3 ColorUtils::HsvToRgb(const Vec3& InHsv) {
    const float hue = std::fmod(std::fmod(InHsv.x, 360.0f) + 360.0f, 360.0f) / 60.0f;
    const float chroma = InHsv.z * InHsv.y;
    const float ramp = chroma * (1.0f - std::fabs(std::fmod(hue, 2.0f) - 1.0f));
    const float base = InHsv.z - chroma;

    switch ((int32_t)hue) {
        case 0:  return Vec3(chroma, ramp, 0.0f) + base;
        case 1:  return Vec3(ramp, chroma, 0.0f) + base;
        case 2:  return Vec3(0.0f, chroma, ramp) + base;
        case 3:  return Vec3(0.0f, ramp, chroma) + base;
        case 4:  return Vec3(ramp, 0.0f, chroma) + base;
        default: return Vec3(chroma, 0.0f, ramp) + base;
    }
}

String ColorUtils::ToHex(const Color& InColor, bool InWithAlpha) {
    const Color clamped = glm::clamp(InColor, Color(0.0f), Color(1.0f));
    const auto byteOf = [](float InValue) { return (int32_t)(InValue * 255.0f + 0.5f); };

    const String rgb = std::format("{:02X}{:02X}{:02X}", byteOf(clamped.r), byteOf(clamped.g), byteOf(clamped.b));
    return InWithAlpha ? rgb + std::format("{:02X}", byteOf(clamped.a)) : rgb;
}

bool ColorUtils::FromHex(const String& InText, Color& OutColor) {
    String digits = InText;
    if (!digits.empty() && digits.front() == '#') {
        digits.erase(0, 1);
    }
    if (digits.size() != 3 && digits.size() != 4 && digits.size() != 6 && digits.size() != 8) {
        return false;
    }

    const bool packed = digits.size() <= 4;
    const size_t componentCount = packed ? digits.size() : digits.size() / 2;

    Color parsed(0.0f, 0.0f, 0.0f, 1.0f);
    for (size_t i = 0; i < componentCount; i++) {
        const int32_t high = HexDigit(digits[packed ? i : i * 2]);
        const int32_t low = HexDigit(digits[packed ? i : i * 2 + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        parsed[(int32_t)i] = (float)(packed ? high * 16 + high : high * 16 + low) / 255.0f;
    }

    OutColor = parsed;
    return true;
}
