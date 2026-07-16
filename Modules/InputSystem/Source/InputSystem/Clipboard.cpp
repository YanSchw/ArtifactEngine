#include "Clipboard.h"

void Clipboard::SetBackend(std::function<String()> InGet, std::function<void(const String&)> InSet) {
    s_Get = std::move(InGet);
    s_Set = std::move(InSet);
}

String Clipboard::GetText() {
    return s_Get ? s_Get() : String();
}

void Clipboard::SetText(const String& InText) {
    if (s_Set) {
        s_Set(InText);
    }
}
