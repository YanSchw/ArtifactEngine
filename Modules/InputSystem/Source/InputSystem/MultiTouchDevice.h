#pragma once
#include "InputDevice.h"
#include "Common/Types.h"
#include "MultiTouchDevice.gen.h"

ARTIFACT_ENUM();
enum class TouchPhase : uint8_t {
    Began,
    Moved,
    Stationary,
    Ended,
    Cancelled
};

struct TouchPoint {
    uint64_t Id = 0;
    TouchPhase Phase = TouchPhase::Began;
    Vec2 Position = Vec2(0.0f);
    Vec2 PreviousPosition = Vec2(0.0f);
    Vec2 StartPosition = Vec2(0.0f);
    float Pressure = 1.0f;
    float Radius = 0.0f;
    double StartTime = 0.0;
    bool BeganThisFrame = false;

    bool IsActive() const { return Phase != TouchPhase::Ended && Phase != TouchPhase::Cancelled; }
    Vec2 GetDelta() const { return Position - PreviousPosition; }
    Vec2 GetTravel() const { return Position - StartPosition; }
    double GetDuration() const;
};

class MultiTouchDevice : public InputDevice {
public:
    ARTIFACT_CLASS();

    static constexpr int32_t AddressableTouches = 10;

    const Array<TouchPoint>& GetTouches() const { return m_Touches; }
    const TouchPoint* FindTouch(uint64_t InId) const;
    const TouchPoint* GetPrimaryTouch() const;
    int32_t GetActiveTouchCount() const;
    bool IsTouching() const { return GetActiveTouchCount() > 0; }

    float GetPinchDelta() const;
    float GetTwistDelta() const;
    Vec2 GetPanDelta() const;

    virtual String GetDeviceName() const override;
    virtual Array<InputControl> GetControls() const override;
    virtual bool ReadControl(const String& InControl, InputValue& OutValue) override;

    static MultiTouchDevice* Instance();
    static double Now();

protected:
    virtual void Tick() override;
    virtual void PollTouches() = 0;

    void BeginTouch(uint64_t InId, const Vec2& InPosition, float InPressure = 1.0f, float InRadius = 0.0f);
    void MoveTouch(uint64_t InId, const Vec2& InPosition, float InPressure = 1.0f, float InRadius = 0.0f);
    void EndTouch(uint64_t InId, const Vec2& InPosition);
    void CancelTouch(uint64_t InId);
    void CancelAllTouches();

private:
    TouchPoint* FindTouchMutable(uint64_t InId);
    bool GetGesturePair(const TouchPoint*& OutA, const TouchPoint*& OutB) const;
    bool ReadIndexedControl(const String& InControl, InputValue& OutValue) const;

    Array<TouchPoint> m_Touches;
};
