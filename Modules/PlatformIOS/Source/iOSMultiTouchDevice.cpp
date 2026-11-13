#include "iOSMultiTouchDevice.h"
#include "iOSSurface.h"

void iOSMultiTouchDevice::PollTouches() {
    iOSSurface* surface = iOSSurface::Get();
    if (!surface) {
        CancelAllTouches();
        return;
    }

    for (const iOSTouchEvent& event : surface->ConsumeTouchEvents()) {
        const Vec2 position = Vec2(event.X, event.Y);
        switch (event.Phase) {
            case iOSTouchPhase::Began:
                BeginTouch(event.Id, position, event.Pressure, event.Radius);
                break;
            case iOSTouchPhase::Moved:
                MoveTouch(event.Id, position, event.Pressure, event.Radius);
                break;
            case iOSTouchPhase::Ended:
                EndTouch(event.Id, position);
                break;
            case iOSTouchPhase::Cancelled:
                CancelTouch(event.Id);
                break;
        }
    }
}
