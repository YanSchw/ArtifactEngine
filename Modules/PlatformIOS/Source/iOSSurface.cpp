#include "iOSSurface.h"
#include "iOSMultiTouchDevice.h"
#include "InputSystem/InputSystem.h"

static iOSSurface* s_Instance = nullptr;

iOSSurface::~iOSSurface() {
    if (s_Instance == this) {
        s_Instance = nullptr;
    }
}

void iOSSurface::Initialize(const SurfaceParams& InParams) {
    (void)InParams;
    s_Instance = this;
    InputSystem::Get().AddDevice(new iOSMultiTouchDevice());
}

uint32_t iOSSurface::GetWidth() const {
    uint32_t width = 0;
    uint32_t height = 0;
    iOSBridge::GetDrawableSize(width, height);
    return width;
}

uint32_t iOSSurface::GetHeight() const {
    uint32_t width = 0;
    uint32_t height = 0;
    iOSBridge::GetDrawableSize(width, height);
    return height;
}

bool iOSSurface::IsMinimized() const {
    return !iOSBridge::IsApplicationActive();
}

void iOSSurface::ProcessEvents() {
    iOSBridge::PumpEvents();
}

void* iOSSurface::GetMetalLayer() const {
    return iOSBridge::GetMetalLayer();
}

void iOSSurface::QueueTouch(const iOSTouchEvent& InEvent) {
    m_TouchEvents.Add(InEvent);
}

Array<iOSTouchEvent> iOSSurface::ConsumeTouchEvents() {
    Array<iOSTouchEvent> events = m_TouchEvents;
    m_TouchEvents.Clear();
    return events;
}

iOSSurface* iOSSurface::Get() {
    return s_Instance;
}

void iOSBridge::SubmitTouch(const iOSTouchEvent& InEvent) {
    if (iOSSurface* surface = iOSSurface::Get()) {
        surface->QueueTouch(InEvent);
    }
}
