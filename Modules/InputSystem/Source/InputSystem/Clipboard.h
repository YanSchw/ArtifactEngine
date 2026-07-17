#pragma once
#include "Common/String.h"
#include <functional>

/** Process-wide text clipboard, bridged to the OS by the active surface backend.
 *  Reads return empty and writes are dropped until then. */
class Clipboard {
public:
    static void SetBackend(std::function<String()> InGet, std::function<void(const String&)> InSet);
    static String GetText();
    static void SetText(const String& InText);

private:
    inline static std::function<String()> s_Get;
    inline static std::function<void(const String&)> s_Set;
};
