#pragma once
#include <cstdint>

enum class iOSTouchPhase : uint8_t {
    Began,
    Moved,
    Ended,
    Cancelled
};

struct iOSTouchEvent {
    uint64_t Id = 0;
    iOSTouchPhase Phase = iOSTouchPhase::Began;
    float X = 0.0f;
    float Y = 0.0f;
    float Pressure = 1.0f;
    float Radius = 0.0f;
};

namespace iOSBridge {
    void* GetMetalLayer();
    void GetDrawableSize(uint32_t& OutWidth, uint32_t& OutHeight);
    bool IsApplicationActive();
    void PumpEvents();

    void SubmitTouch(const iOSTouchEvent& InEvent);
    int RunEngine(int InArgc, char** InArgv);
}
