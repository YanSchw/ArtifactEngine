#include "BrowserMouseDevice.h"
#include "BrowserSurface.h"

void BrowserMouseDevice::Tick() {
    Super::Tick();

    BrowserSurface* surface = BrowserSurface::Get();
    if (!surface) {
        return;
    }

    for (auto& [button, pressed] : m_Buttons) {
        pressed = surface->IsMouseButtonPressed(button);
    }

    m_Position = surface->GetCursorPosition();
    m_ScrollDelta = surface->ConsumeScrollDelta();
}
