#include "BrowserKeyboardDevice.h"
#include "BrowserSurface.h"

void BrowserKeyboardDevice::Tick() {
    Super::Tick();

    BrowserSurface* surface = BrowserSurface::Get();
    if (!surface) {
        return;
    }

    for (auto& [key, pressed] : m_Keys) {
        pressed = surface->IsKeyPressed(key);
    }
}
