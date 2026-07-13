#include "SvgImporter.h"
#include "Common/Map.h"
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <numbers>

namespace {

// ---------------------------------------------------------------------------
// 2x3 affine transform, SVG matrix(a b c d e f) layout.
// ---------------------------------------------------------------------------

struct Xform {
    float a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f, e = 0.0f, f = 0.0f;

    Vec2 Apply(const Vec2& InPoint) const {
        return Vec2(a * InPoint.x + c * InPoint.y + e, b * InPoint.x + d * InPoint.y + f);
    }

    /** this * InOther, i.e. InOther is applied first. */
    Xform Concat(const Xform& InOther) const {
        Xform r;
        r.a = a * InOther.a + c * InOther.b;
        r.b = b * InOther.a + d * InOther.b;
        r.c = a * InOther.c + c * InOther.d;
        r.d = b * InOther.c + d * InOther.d;
        r.e = a * InOther.e + c * InOther.f + e;
        r.f = b * InOther.e + d * InOther.f + f;
        return r;
    }

    Xform Inverted() const {
        const float det = a * d - b * c;
        if (std::abs(det) < 1e-12f) {
            return Xform();
        }
        Xform r;
        r.a = d / det;
        r.b = -b / det;
        r.c = -c / det;
        r.d = a / det;
        r.e = (c * f - d * e) / det;
        r.f = (b * e - a * f) / det;
        return r;
    }

    /** Average length scaling, used to carry stroke widths through transforms. */
    float ScaleFactor() const {
        return std::sqrt(std::abs(a * d - b * c));
    }
};

// ---------------------------------------------------------------------------
// Low-level text scanning
// ---------------------------------------------------------------------------

bool IsSpace(char InChar) {
    return InChar == ' ' || InChar == '\t' || InChar == '\n' || InChar == '\r' || InChar == '\f';
}

void SkipSpace(const char*& InOutCursor) {
    while (IsSpace(*InOutCursor)) {
        InOutCursor++;
    }
}

void SkipSeparators(const char*& InOutCursor) {
    while (IsSpace(*InOutCursor) || *InOutCursor == ',') {
        InOutCursor++;
    }
}

bool ParseNumber(const char*& InOutCursor, float& OutValue) {
    SkipSeparators(InOutCursor);
    char* end = nullptr;
    const float value = strtof(InOutCursor, &end);
    if (end == InOutCursor) {
        return false;
    }
    InOutCursor = end;
    OutValue = value;
    return true;
}

/** Arc flags are single characters and may be packed without separators ("0 1", "01"). */
bool ParseFlag(const char*& InOutCursor, bool& OutValue) {
    SkipSeparators(InOutCursor);
    if (*InOutCursor != '0' && *InOutCursor != '1') {
        return false;
    }
    OutValue = (*InOutCursor == '1');
    InOutCursor++;
    return true;
}

/** Numeric prefix of a length value; the unit suffix (px, pt, %) is ignored. */
float ParseLength(const String& InValue) {
    return strtof(InValue.c_str(), nullptr);
}

/** Length where "50%" means 0.5. */
float ParseFraction(const String& InValue) {
    const char* p = InValue.c_str();
    char* end = nullptr;
    float value = strtof(p, &end);
    if (end != p && *end == '%') {
        value /= 100.0f;
    }
    return value;
}

String Trimmed(const char* InBegin, const char* InEnd) {
    while (InBegin < InEnd && IsSpace(*InBegin)) InBegin++;
    while (InEnd > InBegin && IsSpace(InEnd[-1])) InEnd--;
    return String(InBegin, InEnd);
}

String DecodeEntities(const String& InValue) {
    if (InValue.find('&') == String::npos) {
        return InValue;
    }
    String result;
    result.reserve(InValue.size());
    for (size_t i = 0; i < InValue.size(); i++) {
        if (InValue[i] != '&') {
            result += InValue[i];
            continue;
        }
        const size_t semi = InValue.find(';', i);
        if (semi == String::npos || semi - i > 8) {
            result += InValue[i];
            continue;
        }
        const String entity = InValue.substr(i + 1, semi - i - 1);
        if (entity == "amp") result += '&';
        else if (entity == "lt") result += '<';
        else if (entity == "gt") result += '>';
        else if (entity == "quot") result += '"';
        else if (entity == "apos") result += '\'';
        else if (!entity.empty() && entity[0] == '#') {
            const long code = (entity.size() > 1 && (entity[1] == 'x' || entity[1] == 'X'))
                ? strtol(entity.c_str() + 2, nullptr, 16) : strtol(entity.c_str() + 1, nullptr, 10);
            if (code > 0 && code < 128) result += (char)code;
        } else {
            result += InValue[i];
            continue;
        }
        i = semi;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Colors
// ---------------------------------------------------------------------------

struct NamedColor {
    const char* Name;
    uint32_t Rgb;
};

constexpr NamedColor s_NamedColors[] = {
    { "black", 0x000000 }, { "white", 0xffffff }, { "red", 0xff0000 }, { "green", 0x008000 },
    { "blue", 0x0000ff }, { "yellow", 0xffff00 }, { "cyan", 0x00ffff }, { "aqua", 0x00ffff },
    { "magenta", 0xff00ff }, { "fuchsia", 0xff00ff }, { "gray", 0x808080 }, { "grey", 0x808080 },
    { "silver", 0xc0c0c0 }, { "maroon", 0x800000 }, { "olive", 0x808000 }, { "lime", 0x00ff00 },
    { "teal", 0x008080 }, { "navy", 0x000080 }, { "purple", 0x800080 }, { "orange", 0xffa500 },
    { "gold", 0xffd700 }, { "pink", 0xffc0cb }, { "brown", 0xa52a2a }, { "darkgray", 0xa9a9a9 },
    { "darkgrey", 0xa9a9a9 }, { "lightgray", 0xd3d3d3 }, { "lightgrey", 0xd3d3d3 },
    { "darkred", 0x8b0000 }, { "darkgreen", 0x006400 }, { "darkblue", 0x00008b },
    { "lightblue", 0xadd8e6 }, { "crimson", 0xdc143c }, { "indigo", 0x4b0082 },
    { "violet", 0xee82ee }, { "coral", 0xff7f50 }, { "salmon", 0xfa8072 },
    { "khaki", 0xf0e68c }, { "turquoise", 0x40e0d0 }, { "beige", 0xf5f5dc }
};

int HexDigit(char InChar) {
    if (InChar >= '0' && InChar <= '9') return InChar - '0';
    if (InChar >= 'a' && InChar <= 'f') return InChar - 'a' + 10;
    if (InChar >= 'A' && InChar <= 'F') return InChar - 'A' + 10;
    return -1;
}

Vec4 HslToRgb(float InHue, float InSaturation, float InLightness, float InAlpha) {
    const float h = std::fmod(std::fmod(InHue, 360.0f) + 360.0f, 360.0f) / 60.0f;
    const float c = (1.0f - std::abs(2.0f * InLightness - 1.0f)) * InSaturation;
    const float x = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
    const float m = InLightness - c * 0.5f;
    Vec3 rgb;
    if (h < 1.0f) rgb = Vec3(c, x, 0.0f);
    else if (h < 2.0f) rgb = Vec3(x, c, 0.0f);
    else if (h < 3.0f) rgb = Vec3(0.0f, c, x);
    else if (h < 4.0f) rgb = Vec3(0.0f, x, c);
    else if (h < 5.0f) rgb = Vec3(x, 0.0f, c);
    else rgb = Vec3(c, 0.0f, x);
    return Vec4(rgb + Vec3(m), InAlpha);
}

/** Returns false for unparsable values. Handles hex (3/4/6/8 digits), rgb()/rgba(),
 *  hsl()/hsla(), named colors, transparent and currentColor. */
bool ParseColor(const String& InValue, const Vec4& InCurrentColor, Vec4& OutColor) {
    const char* p = InValue.c_str();
    SkipSpace(p);

    if (*p == '#') {
        p++;
        int digits[8];
        int count = 0;
        while (count < 8 && HexDigit(p[count]) >= 0) {
            digits[count] = HexDigit(p[count]);
            count++;
        }
        if (count == 3 || count == 4) {
            OutColor = Vec4(digits[0] / 15.0f, digits[1] / 15.0f, digits[2] / 15.0f,
                            count == 4 ? digits[3] / 15.0f : 1.0f);
            return true;
        }
        if (count == 6 || count == 8) {
            OutColor = Vec4((digits[0] * 16 + digits[1]) / 255.0f, (digits[2] * 16 + digits[3]) / 255.0f,
                            (digits[4] * 16 + digits[5]) / 255.0f,
                            count == 8 ? (digits[6] * 16 + digits[7]) / 255.0f : 1.0f);
            return true;
        }
        return false;
    }

    if (strncmp(p, "rgb", 3) == 0) {
        p += 3;
        if (*p == 'a') p++;
        SkipSpace(p);
        if (*p != '(') return false;
        p++;
        float channels[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        bool alphaPresent = false;
        for (int i = 0; i < 4; i++) {
            if (!ParseNumber(p, channels[i])) break;
            if (*p == '%') {
                channels[i] = channels[i] * 255.0f / 100.0f;
                p++;
            }
            if (i == 3) alphaPresent = true;
        }
        OutColor = Vec4(channels[0] / 255.0f, channels[1] / 255.0f, channels[2] / 255.0f,
                        alphaPresent ? std::clamp(channels[3], 0.0f, 1.0f) : 1.0f);
        return true;
    }

    if (strncmp(p, "hsl", 3) == 0) {
        p += 3;
        if (*p == 'a') p++;
        SkipSpace(p);
        if (*p != '(') return false;
        p++;
        float values[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        for (int i = 0; i < 4; i++) {
            if (!ParseNumber(p, values[i])) break;
            if (*p == '%') p++;
        }
        OutColor = HslToRgb(values[0], values[1] / 100.0f, values[2] / 100.0f, std::clamp(values[3], 0.0f, 1.0f));
        return true;
    }

    if (InValue == "currentColor") {
        OutColor = InCurrentColor;
        return true;
    }
    if (InValue == "transparent") {
        OutColor = Vec4(0.0f);
        return true;
    }
    for (const NamedColor& named : s_NamedColors) {
        if (InValue == named.Name) {
            OutColor = Vec4(((named.Rgb >> 16) & 0xff) / 255.0f, ((named.Rgb >> 8) & 0xff) / 255.0f,
                            (named.Rgb & 0xff) / 255.0f, 1.0f);
            return true;
        }
    }
    return false;
}

/** Extracts the reference id from "url(#id)" (quotes tolerated). Empty when not a url. */
String ParseUrlReference(const String& InValue) {
    const char* p = InValue.c_str();
    SkipSpace(p);
    if (strncmp(p, "url(", 4) != 0) {
        return String();
    }
    p += 4;
    SkipSpace(p);
    if (*p == '"' || *p == '\'') p++;
    if (*p != '#') {
        return String();
    }
    p++;
    const char* start = p;
    while (*p && *p != ')' && *p != '"' && *p != '\'' && !IsSpace(*p)) {
        p++;
    }
    return String(start, p);
}

// ---------------------------------------------------------------------------
// Transform attribute
// ---------------------------------------------------------------------------

Xform ParseTransform(const String& InValue) {
    Xform result;
    const char* p = InValue.c_str();
    while (*p) {
        SkipSeparators(p);
        const char* word = p;
        while (*p && (isalpha((unsigned char)*p))) {
            p++;
        }
        const size_t wordLength = (size_t)(p - word);
        SkipSpace(p);
        if (wordLength == 0 || *p != '(') {
            break;
        }
        p++;
        float args[6];
        int argCount = 0;
        while (argCount < 6 && ParseNumber(p, args[argCount])) {
            argCount++;
        }
        SkipSeparators(p);
        if (*p == ')') {
            p++;
        }

        auto is = [&](const char* InName) {
            return wordLength == strlen(InName) && strncmp(word, InName, wordLength) == 0;
        };

        Xform t;
        if (is("matrix") && argCount == 6) {
            t.a = args[0]; t.b = args[1]; t.c = args[2]; t.d = args[3]; t.e = args[4]; t.f = args[5];
        } else if (is("translate") && argCount >= 1) {
            t.e = args[0];
            t.f = argCount >= 2 ? args[1] : 0.0f;
        } else if (is("scale") && argCount >= 1) {
            t.a = args[0];
            t.d = argCount >= 2 ? args[1] : args[0];
        } else if (is("rotate") && argCount >= 1) {
            const float rad = glm::radians(args[0]);
            t.a = std::cos(rad); t.b = std::sin(rad); t.c = -std::sin(rad); t.d = std::cos(rad);
            if (argCount >= 3) {
                // rotate(angle cx cy) = translate(cx cy) rotate(angle) translate(-cx -cy)
                t.e = args[1] - (t.a * args[1] + t.c * args[2]);
                t.f = args[2] - (t.b * args[1] + t.d * args[2]);
            }
        } else if (is("skewX") && argCount >= 1) {
            t.c = std::tan(glm::radians(args[0]));
        } else if (is("skewY") && argCount >= 1) {
            t.b = std::tan(glm::radians(args[0]));
        }
        result = result.Concat(t);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Inherited presentation state
// ---------------------------------------------------------------------------

struct SvgState {
    Xform Transform;
    Vec4 CurrentColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

    bool HasFill = true;
    Vec4 FillColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    String FillUrl;
    float FillOpacity = 1.0f;
    SvgFillRule FillRule = SvgFillRule::NonZero;

    bool HasStroke = false;
    Vec4 StrokeColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    String StrokeUrl;
    float StrokeOpacity = 1.0f;
    float StrokeWidth = 1.0f;
    SvgLineCap Cap = SvgLineCap::Butt;
    SvgLineJoin Join = SvgLineJoin::Miter;
    float MiterLimit = 4.0f;

    float Opacity = 1.0f;       // accumulated over ancestors
    float ElementOpacity = 1.0f; // this element's own opacity, folded in once per element
    bool Visible = true;
};

void ApplyPaintProperty(const SvgState& InState, const String& InValue,
                        bool& OutHas, Vec4& OutColor, String& OutUrl) {
    if (InValue == "none") {
        OutHas = false;
        OutUrl.clear();
        return;
    }
    String url = ParseUrlReference(InValue);
    if (!url.empty()) {
        OutHas = true;
        OutUrl = url;
        return;
    }
    Vec4 color;
    if (ParseColor(InValue, InState.CurrentColor, color)) {
        OutHas = true;
        OutColor = color;
        OutUrl.clear();
    }
}

void ApplyDeclarations(SvgState& InOutState, const String& InText);

void ApplyStyleProperty(SvgState& InOutState, const String& InName, const String& InValue) {
    if (InName == "fill") {
        ApplyPaintProperty(InOutState, InValue, InOutState.HasFill, InOutState.FillColor, InOutState.FillUrl);
    } else if (InName == "stroke") {
        ApplyPaintProperty(InOutState, InValue, InOutState.HasStroke, InOutState.StrokeColor, InOutState.StrokeUrl);
    } else if (InName == "color") {
        Vec4 color;
        if (ParseColor(InValue, InOutState.CurrentColor, color)) {
            InOutState.CurrentColor = color;
        }
    } else if (InName == "fill-opacity") {
        InOutState.FillOpacity = std::clamp(ParseFraction(InValue), 0.0f, 1.0f);
    } else if (InName == "stroke-opacity") {
        InOutState.StrokeOpacity = std::clamp(ParseFraction(InValue), 0.0f, 1.0f);
    } else if (InName == "stroke-width") {
        InOutState.StrokeWidth = ParseLength(InValue);
    } else if (InName == "stroke-linecap") {
        if (InValue == "round") InOutState.Cap = SvgLineCap::Round;
        else if (InValue == "square") InOutState.Cap = SvgLineCap::Square;
        else InOutState.Cap = SvgLineCap::Butt;
    } else if (InName == "stroke-linejoin") {
        if (InValue == "round") InOutState.Join = SvgLineJoin::Round;
        else if (InValue == "bevel") InOutState.Join = SvgLineJoin::Bevel;
        else InOutState.Join = SvgLineJoin::Miter;
    } else if (InName == "stroke-miterlimit") {
        InOutState.MiterLimit = std::max(1.0f, ParseLength(InValue));
    } else if (InName == "opacity") {
        InOutState.ElementOpacity = std::clamp(ParseFraction(InValue), 0.0f, 1.0f);
    } else if (InName == "fill-rule") {
        InOutState.FillRule = (InValue == "evenodd") ? SvgFillRule::EvenOdd : SvgFillRule::NonZero;
    } else if (InName == "transform") {
        InOutState.Transform = InOutState.Transform.Concat(ParseTransform(InValue));
    } else if (InName == "display") {
        if (InValue == "none") InOutState.Visible = false;
    } else if (InName == "visibility") {
        if (InValue == "hidden" || InValue == "collapse") InOutState.Visible = false;
    } else if (InName == "style") {
        ApplyDeclarations(InOutState, InValue);
    }
}

/** Applies "name: value; name: value" CSS declaration text. */
void ApplyDeclarations(SvgState& InOutState, const String& InText) {
    const char* p = InText.c_str();
    while (*p) {
        SkipSpace(p);
        const char* nameStart = p;
        while (*p && *p != ':' && *p != ';') p++;
        if (*p != ':') {
            if (*p) p++;
            continue;
        }
        const char* nameEnd = p;
        p++;
        const char* valueStart = p;
        while (*p && *p != ';') p++;
        const char* valueEnd = p;
        if (*p) p++;
        const String name = Trimmed(nameStart, nameEnd);
        if (name != "style") { // guard against recursion on malformed input
            ApplyStyleProperty(InOutState, name, Trimmed(valueStart, valueEnd));
        }
    }
}

// ---------------------------------------------------------------------------
// CSS <style> rules — simple selectors only (element, .class, #id and chains).
// ---------------------------------------------------------------------------

struct CssRule {
    String Element;
    Array<String> Classes;
    String Id;
    String Declarations;
    int Specificity = 0;
};

void ParseCssRules(const String& InCss, Array<CssRule>& OutRules) {
    String css = InCss;
    // Strip /* comments */.
    size_t comment;
    while ((comment = css.find("/*")) != String::npos) {
        const size_t end = css.find("*/", comment + 2);
        css.erase(comment, end == String::npos ? String::npos : end - comment + 2);
    }

    size_t cursor = 0;
    while (cursor < css.size()) {
        const size_t open = css.find('{', cursor);
        if (open == String::npos) break;
        const size_t close = css.find('}', open + 1);
        if (close == String::npos) break;
        const String selectorList = css.substr(cursor, open - cursor);
        const String declarations = css.substr(open + 1, close - open - 1);
        cursor = close + 1;

        const char* s = selectorList.c_str();
        while (*s) {
            const char* start = s;
            while (*s && *s != ',') s++;
            const String selector = Trimmed(start, s);
            if (*s) s++;
            if (selector.empty() || selector.find_first_of(" >+~:[") != String::npos) {
                continue; // combinators and pseudo-selectors are out of scope
            }
            CssRule rule;
            const char* r = selector.c_str();
            const char* tokenStart = r;
            char tokenKind = 'e';
            auto flush = [&](const char* InEnd) {
                const String token(tokenStart, InEnd);
                if (token.empty()) return;
                if (tokenKind == 'e' && token != "*") { rule.Element = token; rule.Specificity += 1; }
                else if (tokenKind == '.') { rule.Classes.Add(token); rule.Specificity += 10; }
                else if (tokenKind == '#') { rule.Id = token; rule.Specificity += 100; }
            };
            while (*r) {
                if (*r == '.' || *r == '#') {
                    flush(r);
                    tokenKind = *r;
                    tokenStart = r + 1;
                }
                r++;
            }
            flush(r);
            rule.Declarations = declarations;
            OutRules.Add(std::move(rule));
        }
    }

    std::sort(OutRules.begin(), OutRules.end(),
              [](const CssRule& InA, const CssRule& InB) { return InA.Specificity < InB.Specificity; });
}

bool ElementHasClass(const String& InClassAttribute, const String& InClass) {
    const char* p = InClassAttribute.c_str();
    while (*p) {
        SkipSpace(p);
        const char* start = p;
        while (*p && !IsSpace(*p)) p++;
        if (InClass == String(start, p)) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// XML element scanning
// ---------------------------------------------------------------------------

struct XmlAttribute {
    String Name;
    String Value;
};

struct XmlElement {
    String Name;
    Array<XmlAttribute> Attributes;
    bool SelfClosing = false;

    const String* Find(const char* InName) const {
        for (const XmlAttribute& attribute : Attributes) {
            if (attribute.Name == InName) {
                return &attribute.Value;
            }
        }
        return nullptr;
    }

    float FindLength(const char* InName, float InDefault) const {
        const String* value = Find(InName);
        return value ? ParseLength(*value) : InDefault;
    }
};

/** Cursor sits just past '<'. Returns false on malformed input; cursor ends past '>'. */
bool ParseElement(const char*& InOutCursor, XmlElement& OutElement) {
    const char* p = InOutCursor;
    const char* nameStart = p;
    while (*p && !IsSpace(*p) && *p != '>' && *p != '/') {
        p++;
    }
    OutElement.Name = String(nameStart, p);
    OutElement.Attributes.Clear();
    OutElement.SelfClosing = false;

    while (*p) {
        SkipSpace(p);
        if (*p == '>') {
            p++;
            break;
        }
        if (*p == '/' || *p == '?') {
            OutElement.SelfClosing = true;
            while (*p && *p != '>') p++;
            if (*p) p++;
            break;
        }
        const char* attrStart = p;
        while (*p && *p != '=' && *p != '>' && !IsSpace(*p)) {
            p++;
        }
        String attrName(attrStart, p);
        SkipSpace(p);
        if (*p != '=') {
            continue;
        }
        p++;
        SkipSpace(p);
        const char quote = *p;
        if (quote != '"' && quote != '\'') {
            return false;
        }
        p++;
        const char* valueStart = p;
        while (*p && *p != quote) {
            p++;
        }
        if (!*p) {
            return false;
        }
        OutElement.Attributes.Add({ std::move(attrName), DecodeEntities(String(valueStart, p)) });
        p++;
    }
    InOutCursor = p;
    return true;
}

/** Cursor sits right after an unclosed open tag; returns the position past the matching
 *  close tag (tag-balance scan, quote-unaware nesting is fine for SVG content). */
const char* FindElementEnd(const char* InCursor) {
    int depth = 1;
    const char* p = InCursor;
    while (*p && depth > 0) {
        if (*p != '<') {
            p++;
            continue;
        }
        if (strncmp(p, "<!--", 4) == 0) {
            const char* end = strstr(p, "-->");
            if (!end) return p + strlen(p);
            p = end + 3;
            continue;
        }
        if (p[1] == '!' || p[1] == '?') {
            while (*p && *p != '>') p++;
            continue;
        }
        if (p[1] == '/') {
            depth--;
            while (*p && *p != '>') p++;
            if (*p) p++;
            continue;
        }
        bool selfClosing = false;
        while (*p && *p != '>') {
            if (*p == '"' || *p == '\'') {
                const char quote = *p;
                p++;
                while (*p && *p != quote) p++;
            }
            if (*p == '/') selfClosing = true;
            if (*p) p++;
        }
        if (*p) p++;
        if (!selfClosing) depth++;
    }
    return p;
}

// ---------------------------------------------------------------------------
// Definitions collected in a pre-pass: CSS rules, gradients, id -> element spans.
// ---------------------------------------------------------------------------

struct GradientDef {
    bool Radial = false;
    bool UserSpace = false;
    bool UnitsSet = false;
    float Coords[4] = { 0.0f, 0.0f, 1.0f, 0.0f }; // x1 y1 x2 y2
    bool CoordSet[4] = { false, false, false, false };
    Xform Transform;
    bool HasTransform = false;
    Array<SvgGradientStop> Stops;
    String Href;
};

struct IdSpan {
    const char* Begin;
    const char* End;
};

struct SvgContext {
    SvgDocument* Document = nullptr;
    Array<CssRule> CssRules;
    Map<String, GradientDef> Gradients;
    Map<String, IdSpan> IdSpans;
    bool RootSeen = false;
    int UseDepth = 0;
};

void CollectGradient(const XmlElement& InElement, GradientDef& OutGradient) {
    OutGradient.Radial = (InElement.Name == "radialGradient");
    if (const String* units = InElement.Find("gradientUnits")) {
        OutGradient.UserSpace = (*units == "userSpaceOnUse");
        OutGradient.UnitsSet = true;
    }
    if (const String* transform = InElement.Find("gradientTransform")) {
        OutGradient.Transform = ParseTransform(*transform);
        OutGradient.HasTransform = true;
    }
    const char* coordNames[4] = { "x1", "y1", "x2", "y2" };
    for (int i = 0; i < 4; i++) {
        if (const String* value = InElement.Find(coordNames[i])) {
            OutGradient.Coords[i] = ParseFraction(*value);
            OutGradient.CoordSet[i] = true;
        }
    }
    if (const String* href = InElement.Find("href")) {
        OutGradient.Href = (!href->empty() && (*href)[0] == '#') ? href->substr(1) : *href;
    } else if (const String* xlink = InElement.Find("xlink:href")) {
        OutGradient.Href = (!xlink->empty() && (*xlink)[0] == '#') ? xlink->substr(1) : *xlink;
    }
}

void CollectGradientStop(const XmlElement& InElement, GradientDef& InOutGradient) {
    SvgGradientStop stop;
    if (const String* offset = InElement.Find("offset")) {
        stop.Offset = std::clamp(ParseFraction(*offset), 0.0f, 1.0f);
    }
    float opacity = 1.0f;
    Vec4 color = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    auto applyStopProperty = [&](const String& InName, const String& InValue) {
        if (InName == "stop-color") {
            Vec4 parsed;
            if (ParseColor(InValue, Vec4(0.0f, 0.0f, 0.0f, 1.0f), parsed)) color = parsed;
        } else if (InName == "stop-opacity") {
            opacity = std::clamp(ParseFraction(InValue), 0.0f, 1.0f);
        }
    };
    for (const XmlAttribute& attribute : InElement.Attributes) {
        if (attribute.Name == "style") {
            const char* p = attribute.Value.c_str();
            while (*p) {
                const char* nameStart = p;
                while (*p && *p != ':' && *p != ';') p++;
                if (*p != ':') { if (*p) p++; continue; }
                const char* nameEnd = p++;
                const char* valueStart = p;
                while (*p && *p != ';') p++;
                applyStopProperty(Trimmed(nameStart, nameEnd), Trimmed(valueStart, p));
                if (*p) p++;
            }
        } else {
            applyStopProperty(attribute.Name, attribute.Value);
        }
    }
    stop.Color = Vec4(Vec3(color), color.a * opacity);
    InOutGradient.Stops.Add(stop);
}

/** Pre-pass over the whole text: gathers <style> CSS, gradient definitions (including
 *  stops), and the text span of every element carrying an id (for <use>). */
void CollectDefinitions(const char* InBegin, SvgContext& InOutContext) {
    const char* p = InBegin;
    String currentGradientId;
    while (*p) {
        if (*p != '<') {
            p++;
            continue;
        }
        const char* tagStart = p;
        p++;
        if (strncmp(p, "!--", 3) == 0) {
            const char* end = strstr(p, "-->");
            if (!end) return;
            p = end + 3;
            continue;
        }
        if (*p == '!' || *p == '?') {
            while (*p && *p != '>') p++;
            if (*p) p++;
            continue;
        }
        if (*p == '/') {
            XmlElement closing;
            const char* nameStart = ++p;
            while (*p && *p != '>' && !IsSpace(*p)) p++;
            const String name(nameStart, p);
            while (*p && *p != '>') p++;
            if (*p) p++;
            if (name == "linearGradient" || name == "radialGradient") {
                currentGradientId.clear();
            }
            continue;
        }

        XmlElement element;
        if (!ParseElement(p, element)) {
            return;
        }

        if (element.Name == "style") {
            const char* cssStart = p;
            const char* cssEnd = strstr(p, "</style");
            if (!cssEnd) return;
            String css(cssStart, cssEnd);
            const size_t cdata = css.find("<![CDATA[");
            if (cdata != String::npos) {
                css.erase(cdata, 9);
                const size_t cdataEnd = css.find("]]>");
                if (cdataEnd != String::npos) css.erase(cdataEnd, 3);
            }
            ParseCssRules(css, InOutContext.CssRules);
            p = cssEnd;
            continue;
        }

        if (element.Name == "linearGradient" || element.Name == "radialGradient") {
            currentGradientId.clear();
            if (const String* id = element.Find("id")) {
                GradientDef gradient;
                CollectGradient(element, gradient);
                InOutContext.Gradients[*id] = gradient;
                if (!element.SelfClosing) {
                    currentGradientId = *id;
                }
            }
            continue;
        }
        if (element.Name == "stop" && !currentGradientId.empty()) {
            CollectGradientStop(element, InOutContext.Gradients[currentGradientId]);
            continue;
        }

        if (const String* id = element.Find("id")) {
            const char* spanEnd = element.SelfClosing ? p : FindElementEnd(p);
            if (!InOutContext.IdSpans.ContainsKey(*id)) {
                InOutContext.IdSpans[*id] = { tagStart, spanEnd };
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Gradient resolution
// ---------------------------------------------------------------------------

Vec4 AverageStopColor(const Array<SvgGradientStop>& InStops) {
    if (InStops.IsEmpty()) {
        return Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    if (InStops.Size() == 1) {
        return InStops[0].Color;
    }
    Vec4 total = Vec4(0.0f);
    float span = 0.0f;
    for (int32_t i = 0; i + 1 < InStops.Size(); i++) {
        const float width = std::max(InStops[i + 1].Offset - InStops[i].Offset, 0.0f);
        total += (InStops[i].Color + InStops[i + 1].Color) * 0.5f * width;
        span += width;
    }
    return span > 1e-6f ? total / span : InStops[0].Color;
}

/** Follows href chains to a def with stops/coords, then bakes the gradient into document
 *  space for the given shape. Radial gradients degrade to their average color. */
bool ResolveGradient(SvgContext& InContext, const String& InId, const SvgState& InState,
                     float InAlphaMultiplier, const Array<SvgContour>& InContours,
                     Vec4& OutFlatColor, SvgLinearGradient& OutGradient, bool& OutIsGradient) {
    GradientDef merged;
    bool mergedValid = false;
    String id = InId;
    for (int depth = 0; depth < 4 && !id.empty(); depth++) {
        if (!InContext.Gradients.ContainsKey(id)) {
            break;
        }
        const GradientDef& def = InContext.Gradients.At(id);
        if (!mergedValid) {
            merged = def;
            mergedValid = true;
        } else {
            if (merged.Stops.IsEmpty()) merged.Stops = def.Stops;
            if (!merged.HasTransform && def.HasTransform) { merged.Transform = def.Transform; merged.HasTransform = true; }
            if (!merged.UnitsSet && def.UnitsSet) { merged.UserSpace = def.UserSpace; merged.UnitsSet = true; }
            for (int i = 0; i < 4; i++) {
                if (!merged.CoordSet[i] && def.CoordSet[i]) { merged.Coords[i] = def.Coords[i]; merged.CoordSet[i] = true; }
            }
        }
        id = def.Href;
        if (id == InId) break; // self-reference guard
    }
    if (!mergedValid || merged.Stops.IsEmpty()) {
        return false;
    }

    Array<SvgGradientStop> stops = merged.Stops;
    std::sort(stops.begin(), stops.end(),
              [](const SvgGradientStop& InA, const SvgGradientStop& InB) { return InA.Offset < InB.Offset; });
    for (SvgGradientStop& stop : stops) {
        stop.Color.a *= InAlphaMultiplier;
    }

    OutFlatColor = AverageStopColor(stops);
    OutIsGradient = false;
    if (merged.Radial || stops.Size() < 2) {
        return true;
    }

    // G maps gradient coordinates to document space; T(p) is then affine in p.
    Xform unitsToDoc;
    if (merged.UserSpace) {
        unitsToDoc = InState.Transform;
    } else {
        Vec2 boundsMin(1e30f), boundsMax(-1e30f);
        for (const SvgContour& contour : InContours) {
            for (const Vec2& point : contour.Points) {
                boundsMin = glm::min(boundsMin, point);
                boundsMax = glm::max(boundsMax, point);
            }
        }
        if (boundsMax.x <= boundsMin.x || boundsMax.y <= boundsMin.y) {
            return true;
        }
        unitsToDoc.a = boundsMax.x - boundsMin.x;
        unitsToDoc.d = boundsMax.y - boundsMin.y;
        unitsToDoc.e = boundsMin.x;
        unitsToDoc.f = boundsMin.y;
    }
    Xform gradientToDoc = merged.HasTransform ? unitsToDoc.Concat(merged.Transform) : unitsToDoc;
    const Xform inverse = gradientToDoc.Inverted();

    const Vec2 p0 = Vec2(merged.Coords[0], merged.Coords[1]);
    const Vec2 delta = Vec2(merged.Coords[2], merged.Coords[3]) - p0;
    const float deltaSq = delta.x * delta.x + delta.y * delta.y;
    if (deltaSq < 1e-12f) {
        return true;
    }
    OutGradient.TCoeffs = Vec3(
        (inverse.a * delta.x + inverse.b * delta.y) / deltaSq,
        (inverse.c * delta.x + inverse.d * delta.y) / deltaSq,
        ((inverse.e - p0.x) * delta.x + (inverse.f - p0.y) * delta.y) / deltaSq);
    OutGradient.Stops = std::move(stops);
    OutIsGradient = true;
    return true;
}

// ---------------------------------------------------------------------------
// Path building — every segment is stored as a cubic so contours stay uniform.
// ---------------------------------------------------------------------------

struct PathBuilder {
    SvgShape* Shape = nullptr;
    const Xform* Transform = nullptr;
    Vec2 Position = Vec2(0.0f);

    void MoveTo(const Vec2& InPoint) {
        SvgContour contour;
        contour.Points.Add(Transform->Apply(InPoint));
        Shape->Contours.Add(contour);
        Position = InPoint;
    }

    void CubicTo(const Vec2& InControlA, const Vec2& InControlB, const Vec2& InEnd) {
        if (Shape->Contours.IsEmpty()) {
            MoveTo(Position);
        }
        Array<Vec2>& points = Shape->Contours.LastItem().Points;
        points.Add(Transform->Apply(InControlA));
        points.Add(Transform->Apply(InControlB));
        points.Add(Transform->Apply(InEnd));
        Position = InEnd;
    }

    void LineTo(const Vec2& InPoint) {
        CubicTo(Position + (InPoint - Position) / 3.0f, Position + (InPoint - Position) * (2.0f / 3.0f), InPoint);
    }

    void QuadTo(const Vec2& InControl, const Vec2& InEnd) {
        CubicTo(Position + (InControl - Position) * (2.0f / 3.0f), InEnd + (InControl - InEnd) * (2.0f / 3.0f), InEnd);
    }

    void Close() {
        if (!Shape->Contours.IsEmpty()) {
            Shape->Contours.LastItem().Closed = true;
        }
    }
};

/** Elliptical arc -> cubic segments (SVG spec F.6.5 endpoint to center conversion). */
void ArcTo(PathBuilder& InOutBuilder, float InRadiusX, float InRadiusY, float InAxisRotationDegrees,
           bool InLargeArc, bool InSweep, const Vec2& InEnd) {
    const Vec2 start = InOutBuilder.Position;
    float rx = std::abs(InRadiusX);
    float ry = std::abs(InRadiusY);
    if (rx < 1e-6f || ry < 1e-6f || glm::distance(start, InEnd) < 1e-6f) {
        InOutBuilder.LineTo(InEnd);
        return;
    }

    const float phi = glm::radians(InAxisRotationDegrees);
    const float cosPhi = std::cos(phi);
    const float sinPhi = std::sin(phi);

    const Vec2 half = (start - InEnd) * 0.5f;
    const Vec2 p = Vec2(cosPhi * half.x + sinPhi * half.y, -sinPhi * half.x + cosPhi * half.y);

    const float lambda = (p.x * p.x) / (rx * rx) + (p.y * p.y) / (ry * ry);
    if (lambda > 1.0f) {
        const float s = std::sqrt(lambda);
        rx *= s;
        ry *= s;
    }

    float num = rx * rx * ry * ry - rx * rx * p.y * p.y - ry * ry * p.x * p.x;
    const float den = rx * rx * p.y * p.y + ry * ry * p.x * p.x;
    num = std::max(num, 0.0f);
    float scale = std::sqrt(num / den);
    if (InLargeArc == InSweep) {
        scale = -scale;
    }
    const Vec2 centerPrime = Vec2(scale * rx * p.y / ry, -scale * ry * p.x / rx);
    const Vec2 mid = (start + InEnd) * 0.5f;
    const Vec2 center = Vec2(cosPhi * centerPrime.x - sinPhi * centerPrime.y + mid.x,
                             sinPhi * centerPrime.x + cosPhi * centerPrime.y + mid.y);

    auto angleOf = [](const Vec2& InVec) { return std::atan2(InVec.y, InVec.x); };
    const Vec2 startVec = Vec2((p.x - centerPrime.x) / rx, (p.y - centerPrime.y) / ry);
    const Vec2 endVec = Vec2((-p.x - centerPrime.x) / rx, (-p.y - centerPrime.y) / ry);
    const float startAngle = angleOf(startVec);
    float sweepAngle = angleOf(endVec) - startAngle;
    const float twoPi = 2.0f * std::numbers::pi_v<float>;
    if (!InSweep && sweepAngle > 0.0f) {
        sweepAngle -= twoPi;
    } else if (InSweep && sweepAngle < 0.0f) {
        sweepAngle += twoPi;
    }

    const int segments = std::max(1, (int)std::ceil(std::abs(sweepAngle) / (twoPi * 0.25f)));
    const float delta = sweepAngle / (float)segments;
    // Control point distance for a cubic approximating a delta-radian arc.
    const float k = (4.0f / 3.0f) * std::tan(delta * 0.25f);

    auto pointAt = [&](float InAngle, Vec2& OutPoint, Vec2& OutDerivative) {
        const float cosA = std::cos(InAngle);
        const float sinA = std::sin(InAngle);
        OutPoint = Vec2(center.x + rx * cosA * cosPhi - ry * sinA * sinPhi,
                        center.y + rx * cosA * sinPhi + ry * sinA * cosPhi);
        OutDerivative = Vec2(-rx * sinA * cosPhi - ry * cosA * sinPhi,
                             -rx * sinA * sinPhi + ry * cosA * cosPhi);
    };

    Vec2 point, derivative;
    pointAt(startAngle, point, derivative);
    for (int i = 1; i <= segments; i++) {
        Vec2 nextPoint, nextDerivative;
        pointAt(startAngle + delta * (float)i, nextPoint, nextDerivative);
        const Vec2 end = (i == segments) ? InEnd : nextPoint;
        InOutBuilder.CubicTo(point + derivative * k, end - nextDerivative * k, end);
        point = nextPoint;
        derivative = nextDerivative;
    }
}

void ParsePathData(const String& InData, SvgShape& OutShape, const Xform& InTransform) {
    PathBuilder builder;
    builder.Shape = &OutShape;
    builder.Transform = &InTransform;

    const char* p = InData.c_str();
    char command = 0;
    Vec2 contourStart = Vec2(0.0f);
    Vec2 previousCubicControl = Vec2(0.0f);
    Vec2 previousQuadControl = Vec2(0.0f);
    char previousCommand = 0;

    while (*p) {
        SkipSeparators(p);
        if (!*p) {
            break;
        }
        if (isalpha((unsigned char)*p)) {
            command = *p;
            p++;
        } else if (command == 0) {
            break;
        } else if (command == 'M') {
            command = 'L'; // subsequent implicit pairs after moveto are lineto
        } else if (command == 'm') {
            command = 'l';
        }

        const bool relative = (command >= 'a' && command <= 'z');
        const Vec2 origin = relative ? builder.Position : Vec2(0.0f);
        const char upper = (char)toupper((unsigned char)command);
        float n[7];

        switch (upper) {
        case 'M': {
            if (!ParseNumber(p, n[0]) || !ParseNumber(p, n[1])) return;
            builder.MoveTo(origin + Vec2(n[0], n[1]));
            contourStart = builder.Position;
            break;
        }
        case 'L': {
            if (!ParseNumber(p, n[0]) || !ParseNumber(p, n[1])) return;
            builder.LineTo(origin + Vec2(n[0], n[1]));
            break;
        }
        case 'H': {
            if (!ParseNumber(p, n[0])) return;
            builder.LineTo(Vec2(origin.x + n[0], builder.Position.y));
            break;
        }
        case 'V': {
            if (!ParseNumber(p, n[0])) return;
            builder.LineTo(Vec2(builder.Position.x, origin.y + n[0]));
            break;
        }
        case 'C': {
            for (int i = 0; i < 6; i++) {
                if (!ParseNumber(p, n[i])) return;
            }
            previousCubicControl = origin + Vec2(n[2], n[3]);
            builder.CubicTo(origin + Vec2(n[0], n[1]), previousCubicControl, origin + Vec2(n[4], n[5]));
            break;
        }
        case 'S': {
            for (int i = 0; i < 4; i++) {
                if (!ParseNumber(p, n[i])) return;
            }
            const char prevUpper = (char)toupper((unsigned char)previousCommand);
            const Vec2 reflected = (prevUpper == 'C' || prevUpper == 'S')
                ? builder.Position * 2.0f - previousCubicControl : builder.Position;
            previousCubicControl = origin + Vec2(n[0], n[1]);
            builder.CubicTo(reflected, previousCubicControl, origin + Vec2(n[2], n[3]));
            break;
        }
        case 'Q': {
            for (int i = 0; i < 4; i++) {
                if (!ParseNumber(p, n[i])) return;
            }
            previousQuadControl = origin + Vec2(n[0], n[1]);
            builder.QuadTo(previousQuadControl, origin + Vec2(n[2], n[3]));
            break;
        }
        case 'T': {
            if (!ParseNumber(p, n[0]) || !ParseNumber(p, n[1])) return;
            const char prevUpper = (char)toupper((unsigned char)previousCommand);
            previousQuadControl = (prevUpper == 'Q' || prevUpper == 'T')
                ? builder.Position * 2.0f - previousQuadControl : builder.Position;
            builder.QuadTo(previousQuadControl, origin + Vec2(n[0], n[1]));
            break;
        }
        case 'A': {
            bool largeArc = false, sweep = false;
            if (!ParseNumber(p, n[0]) || !ParseNumber(p, n[1]) || !ParseNumber(p, n[2]) ||
                !ParseFlag(p, largeArc) || !ParseFlag(p, sweep) ||
                !ParseNumber(p, n[3]) || !ParseNumber(p, n[4])) return;
            ArcTo(builder, n[0], n[1], n[2], largeArc, sweep, origin + Vec2(n[3], n[4]));
            break;
        }
        case 'Z': {
            builder.Close();
            builder.Position = contourStart;
            break;
        }
        default:
            return;
        }
        previousCommand = command;
    }
}

// ---------------------------------------------------------------------------
// Basic shape elements, expressed through the same cubic contours
// ---------------------------------------------------------------------------

void BuildEllipse(SvgShape& OutShape, const Vec2& InCenter, const Vec2& InRadii, const Xform& InTransform) {
    // Kappa: control distance approximating a quarter circle with a cubic.
    constexpr float kappa = 0.5522847493f;
    const Vec2 k = InRadii * kappa;
    PathBuilder builder;
    builder.Shape = &OutShape;
    builder.Transform = &InTransform;
    const Vec2 c = InCenter;
    const Vec2 r = InRadii;
    builder.MoveTo(c + Vec2(r.x, 0.0f));
    builder.CubicTo(c + Vec2(r.x, k.y), c + Vec2(k.x, r.y), c + Vec2(0.0f, r.y));
    builder.CubicTo(c + Vec2(-k.x, r.y), c + Vec2(-r.x, k.y), c + Vec2(-r.x, 0.0f));
    builder.CubicTo(c + Vec2(-r.x, -k.y), c + Vec2(-k.x, -r.y), c + Vec2(0.0f, -r.y));
    builder.CubicTo(c + Vec2(k.x, -r.y), c + Vec2(r.x, -k.y), c + Vec2(r.x, 0.0f));
    builder.Close();
}

void BuildRect(SvgShape& OutShape, const Vec2& InPosition, const Vec2& InSize, Vec2 InCornerRadii, const Xform& InTransform) {
    PathBuilder builder;
    builder.Shape = &OutShape;
    builder.Transform = &InTransform;
    InCornerRadii = glm::clamp(InCornerRadii, Vec2(0.0f), InSize * 0.5f);
    const Vec2 mn = InPosition;
    const Vec2 mx = InPosition + InSize;
    if (InCornerRadii.x < 1e-4f || InCornerRadii.y < 1e-4f) {
        builder.MoveTo(mn);
        builder.LineTo(Vec2(mx.x, mn.y));
        builder.LineTo(mx);
        builder.LineTo(Vec2(mn.x, mx.y));
        builder.Close();
        return;
    }
    constexpr float kappa = 0.5522847493f;
    const Vec2 r = InCornerRadii;
    const Vec2 k = r * (1.0f - kappa);
    builder.MoveTo(Vec2(mn.x + r.x, mn.y));
    builder.LineTo(Vec2(mx.x - r.x, mn.y));
    builder.CubicTo(Vec2(mx.x - k.x, mn.y), Vec2(mx.x, mn.y + k.y), Vec2(mx.x, mn.y + r.y));
    builder.LineTo(Vec2(mx.x, mx.y - r.y));
    builder.CubicTo(Vec2(mx.x, mx.y - k.y), Vec2(mx.x - k.x, mx.y), Vec2(mx.x - r.x, mx.y));
    builder.LineTo(Vec2(mn.x + r.x, mx.y));
    builder.CubicTo(Vec2(mn.x + k.x, mx.y), Vec2(mn.x, mx.y - k.y), Vec2(mn.x, mx.y - r.y));
    builder.LineTo(Vec2(mn.x, mn.y + r.y));
    builder.CubicTo(Vec2(mn.x, mn.y + k.y), Vec2(mn.x + k.x, mn.y), Vec2(mn.x + r.x, mn.y));
    builder.Close();
}

void BuildPolyline(SvgShape& OutShape, const String& InPoints, bool InClosed, const Xform& InTransform) {
    PathBuilder builder;
    builder.Shape = &OutShape;
    builder.Transform = &InTransform;
    const char* p = InPoints.c_str();
    float x, y;
    bool first = true;
    while (ParseNumber(p, x) && ParseNumber(p, y)) {
        if (first) {
            builder.MoveTo(Vec2(x, y));
            first = false;
        } else {
            builder.LineTo(Vec2(x, y));
        }
    }
    if (InClosed) {
        builder.Close();
    }
}

// ---------------------------------------------------------------------------
// Element traversal
// ---------------------------------------------------------------------------

bool IsSkippedSubtree(const String& InName) {
    return InName == "defs" || InName == "symbol" || InName == "clipPath" || InName == "mask" ||
           InName == "marker" || InName == "pattern" || InName == "metadata" || InName == "style" ||
           InName == "text" || InName == "filter" || InName == "linearGradient" || InName == "radialGradient";
}

void ApplyElementState(const XmlElement& InElement, const SvgContext& InContext, SvgState& InOutState) {
    // Priority (lowest to highest): presentation attributes, CSS rules by specificity,
    // then the inline style attribute.
    InOutState.ElementOpacity = 1.0f;
    for (const XmlAttribute& attribute : InElement.Attributes) {
        if (attribute.Name != "style") {
            ApplyStyleProperty(InOutState, attribute.Name, attribute.Value);
        }
    }

    if (!InContext.CssRules.IsEmpty()) {
        const String* classAttribute = InElement.Find("class");
        const String* idAttribute = InElement.Find("id");
        for (const CssRule& rule : InContext.CssRules) {
            if (!rule.Element.empty() && rule.Element != InElement.Name) continue;
            if (!rule.Id.empty() && (!idAttribute || rule.Id != *idAttribute)) continue;
            bool classesMatch = true;
            for (const String& cssClass : rule.Classes) {
                if (!classAttribute || !ElementHasClass(*classAttribute, cssClass)) {
                    classesMatch = false;
                    break;
                }
            }
            if (classesMatch) {
                ApplyDeclarations(InOutState, rule.Declarations);
            }
        }
    }

    if (const String* style = InElement.Find("style")) {
        ApplyDeclarations(InOutState, *style);
    }
    InOutState.Opacity *= InOutState.ElementOpacity;
}

void EmitShape(const XmlElement& InElement, const SvgState& InState, SvgContext& InOutContext) {
    SvgShape shape;
    shape.FillRule = InState.FillRule;
    const float groupAlpha = InState.Opacity;
    const bool fillable = InState.HasFill && (InState.FillColor.a * InState.FillOpacity * groupAlpha > 0.0f || !InState.FillUrl.empty());
    const bool strokeable = InState.HasStroke && InState.StrokeWidth > 0.0f &&
                            (InState.StrokeColor.a * InState.StrokeOpacity * groupAlpha > 0.0f || !InState.StrokeUrl.empty());
    if (!fillable && !strokeable) {
        return;
    }

    auto attr = [&](const char* InName, float InDefault) { return InElement.FindLength(InName, InDefault); };

    if (InElement.Name == "path") {
        if (const String* data = InElement.Find("d")) {
            ParsePathData(*data, shape, InState.Transform);
        }
    } else if (InElement.Name == "rect") {
        const Vec2 size = Vec2(attr("width", 0.0f), attr("height", 0.0f));
        float rx = attr("rx", -1.0f);
        float ry = attr("ry", -1.0f);
        if (rx < 0.0f) rx = std::max(ry, 0.0f);
        if (ry < 0.0f) ry = std::max(rx, 0.0f);
        if (size.x > 0.0f && size.y > 0.0f) {
            BuildRect(shape, Vec2(attr("x", 0.0f), attr("y", 0.0f)), size, Vec2(rx, ry), InState.Transform);
        }
    } else if (InElement.Name == "circle") {
        const float radius = attr("r", 0.0f);
        if (radius > 0.0f) {
            BuildEllipse(shape, Vec2(attr("cx", 0.0f), attr("cy", 0.0f)), Vec2(radius), InState.Transform);
        }
    } else if (InElement.Name == "ellipse") {
        const Vec2 radii = Vec2(attr("rx", 0.0f), attr("ry", 0.0f));
        if (radii.x > 0.0f && radii.y > 0.0f) {
            BuildEllipse(shape, Vec2(attr("cx", 0.0f), attr("cy", 0.0f)), radii, InState.Transform);
        }
    } else if (InElement.Name == "line") {
        PathBuilder builder;
        builder.Shape = &shape;
        builder.Transform = &InState.Transform;
        builder.MoveTo(Vec2(attr("x1", 0.0f), attr("y1", 0.0f)));
        builder.LineTo(Vec2(attr("x2", 0.0f), attr("y2", 0.0f)));
    } else if (InElement.Name == "polygon" || InElement.Name == "polyline") {
        if (const String* points = InElement.Find("points")) {
            BuildPolyline(shape, *points, InElement.Name == "polygon", InState.Transform);
        }
    } else {
        return;
    }

    if (shape.Contours.IsEmpty()) {
        return;
    }

    // Lines never fill; polylines do (as if closed), matching the spec.
    shape.HasFill = fillable && InElement.Name != "line";
    if (shape.HasFill) {
        const float alphaMultiplier = InState.FillOpacity * groupAlpha;
        if (!InState.FillUrl.empty()) {
            if (!ResolveGradient(InOutContext, InState.FillUrl, InState, alphaMultiplier, shape.Contours,
                                 shape.FillColor, shape.FillGradient, shape.HasFillGradient)) {
                shape.HasFill = false; // unresolved reference: skip rather than paint black
            }
        } else {
            shape.FillColor = Vec4(Vec3(InState.FillColor), InState.FillColor.a * alphaMultiplier);
        }
        if (shape.HasFill && shape.FillColor.a <= 0.0f && !shape.HasFillGradient) {
            shape.HasFill = false;
        }
    }

    shape.HasStroke = strokeable;
    if (shape.HasStroke) {
        const float alphaMultiplier = InState.StrokeOpacity * groupAlpha;
        if (!InState.StrokeUrl.empty()) {
            SvgLinearGradient unusedGradient;
            bool unusedIsGradient = false;
            if (!ResolveGradient(InOutContext, InState.StrokeUrl, InState, alphaMultiplier, shape.Contours,
                                 shape.StrokeColor, unusedGradient, unusedIsGradient)) {
                shape.HasStroke = false;
            }
        } else {
            shape.StrokeColor = Vec4(Vec3(InState.StrokeColor), InState.StrokeColor.a * alphaMultiplier);
        }
        shape.StrokeWidth = InState.StrokeWidth * InState.Transform.ScaleFactor();
        shape.Cap = InState.Cap;
        shape.Join = InState.Join;
        shape.MiterLimit = InState.MiterLimit;
        if (shape.StrokeColor.a <= 0.0f || shape.StrokeWidth <= 0.0f) {
            shape.HasStroke = false;
        }
    }

    if (shape.HasFill || shape.HasStroke) {
        InOutContext.Document->Shapes.Add(std::move(shape));
    }
}

/** Walks the element range and emits shapes; recurses for <use> replays. */
void ParseRange(const char* InBegin, const char* InEnd, const SvgState& InBaseState, SvgContext& InOutContext) {
    Array<SvgState> stack;
    stack.Add(InBaseState);
    int skipDepth = 0;

    const char* p = InBegin;
    while (p < InEnd && *p) {
        if (*p != '<') {
            p++;
            continue;
        }
        p++;
        if (strncmp(p, "!--", 3) == 0) {
            const char* end = strstr(p, "-->");
            if (!end) return;
            p = end + 3;
            continue;
        }
        if (*p == '!' || *p == '?') {
            while (p < InEnd && *p != '>') p++;
            if (p < InEnd) p++;
            continue;
        }
        if (*p == '/') {
            p++;
            while (p < InEnd && *p != '>') p++;
            if (p < InEnd) p++;
            if (skipDepth > 0) {
                skipDepth--;
            } else if (stack.Size() > 1) {
                stack.RemoveLastItem();
            }
            continue;
        }

        XmlElement element;
        if (!ParseElement(p, element)) {
            return;
        }
        if (skipDepth > 0) {
            if (!element.SelfClosing) skipDepth++;
            continue;
        }
        // A <use> replay may target a <symbol>, which then behaves like a group.
        const bool symbolReplay = (element.Name == "symbol" && InOutContext.UseDepth > 0);
        if (IsSkippedSubtree(element.Name) && !symbolReplay) {
            if (!element.SelfClosing) skipDepth++;
            continue;
        }

        SvgState state = stack.LastItem();
        ApplyElementState(element, InOutContext, state);

        if (element.Name == "svg" && !InOutContext.RootSeen) {
            InOutContext.RootSeen = true;
            if (const String* viewBox = element.Find("viewBox")) {
                const char* v = viewBox->c_str();
                float box[4];
                if (ParseNumber(v, box[0]) && ParseNumber(v, box[1]) && ParseNumber(v, box[2]) && ParseNumber(v, box[3])) {
                    InOutContext.Document->Width = box[2];
                    InOutContext.Document->Height = box[3];
                    Xform shift;
                    shift.e = -box[0];
                    shift.f = -box[1];
                    state.Transform = state.Transform.Concat(shift);
                }
            }
            if (InOutContext.Document->Width <= 0.0f) {
                if (const String* width = element.Find("width")) InOutContext.Document->Width = ParseLength(*width);
                if (const String* height = element.Find("height")) InOutContext.Document->Height = ParseLength(*height);
            }
        } else if (state.Visible && element.Name == "use") {
            const String* href = element.Find("href");
            if (!href) href = element.Find("xlink:href");
            if (href && !href->empty() && (*href)[0] == '#' && InOutContext.UseDepth < 4) {
                const String id = href->substr(1);
                if (InOutContext.IdSpans.ContainsKey(id)) {
                    const IdSpan span = InOutContext.IdSpans.At(id);
                    Xform shift;
                    shift.e = element.FindLength("x", 0.0f);
                    shift.f = element.FindLength("y", 0.0f);
                    SvgState useState = state;
                    useState.Transform = useState.Transform.Concat(shift);
                    InOutContext.UseDepth++;
                    ParseRange(span.Begin, span.End, useState, InOutContext);
                    InOutContext.UseDepth--;
                }
            }
        } else if (state.Visible) {
            EmitShape(element, state, InOutContext);
        }

        if (!element.SelfClosing) {
            stack.Add(state);
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------

Vec4 SvgLinearGradient::Sample(const Vec2& InPoint) const {
    if (Stops.IsEmpty()) {
        return Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    const float t = std::clamp(TCoeffs.x * InPoint.x + TCoeffs.y * InPoint.y + TCoeffs.z, 0.0f, 1.0f);
    if (t <= Stops[0].Offset) {
        return Stops[0].Color;
    }
    for (int32_t i = 0; i + 1 < Stops.Size(); i++) {
        if (t <= Stops[i + 1].Offset) {
            const float span = Stops[i + 1].Offset - Stops[i].Offset;
            const float blend = span > 1e-6f ? (t - Stops[i].Offset) / span : 0.0f;
            return glm::mix(Stops[i].Color, Stops[i + 1].Color, blend);
        }
    }
    return Stops[Stops.Size() - 1].Color;
}

bool SvgImporter::Parse(const String& InSvgText, SvgDocument& OutDocument) {
    OutDocument = SvgDocument();

    SvgContext context;
    context.Document = &OutDocument;
    CollectDefinitions(InSvgText.c_str(), context);
    ParseRange(InSvgText.c_str(), InSvgText.c_str() + InSvgText.size(), SvgState(), context);

    // Documents without viewBox/width fall back to the geometry's extent.
    if (OutDocument.Width <= 0.0f || OutDocument.Height <= 0.0f) {
        Vec2 maxPoint = Vec2(0.0f);
        for (const SvgShape& shape : OutDocument.Shapes) {
            for (const SvgContour& contour : shape.Contours) {
                for (const Vec2& point : contour.Points) {
                    maxPoint = glm::max(maxPoint, point);
                }
            }
        }
        if (OutDocument.Width <= 0.0f) OutDocument.Width = maxPoint.x;
        if (OutDocument.Height <= 0.0f) OutDocument.Height = maxPoint.y;
    }

    return context.RootSeen && OutDocument.Width > 0.0f && OutDocument.Height > 0.0f;
}
