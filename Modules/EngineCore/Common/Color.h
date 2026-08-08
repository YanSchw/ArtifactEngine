#pragma once
#include "Types.h"

namespace ColorUtils {
    Vec3 RgbToHsv(const Vec3& InRgb);
    Vec3 HsvToRgb(const Vec3& InHsv);

    String ToHex(const Color& InColor, bool InWithAlpha = true);
    bool FromHex(const String& InText, Color& OutColor);
}
