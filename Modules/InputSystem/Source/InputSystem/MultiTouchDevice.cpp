#include "MultiTouchDevice.h"
#include "InputSystem.h"

#include <chrono>

static constexpr const char* s_TouchPrefix = "Touch";

static InputValue MakeValue(float InX, float InY = 0.0f) {
    InputValue value;
    value.Raw = Vec2(InX, InY);
    return value;
}

static InputValue MakeValue(const Vec2& InVector) {
    return MakeValue(InVector.x, InVector.y);
}

static float WrapAngle(float InRadians) {
    return std::atan2(std::sin(InRadians), std::cos(InRadians));
}

double TouchPoint::GetDuration() const {
    return MultiTouchDevice::Now() - StartTime;
}

double MultiTouchDevice::Now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

const TouchPoint* MultiTouchDevice::FindTouch(uint64_t InId) const {
    const TouchPoint* released = nullptr;
    for (const TouchPoint& touch : m_Touches) {
        if (touch.Id != InId) {
            continue;
        }
        if (touch.IsActive()) {
            return &touch;
        }
        released = &touch;
    }
    return released;
}

const TouchPoint* MultiTouchDevice::GetPrimaryTouch() const {
    return m_Touches.IsEmpty() ? nullptr : &m_Touches[0];
}

int32_t MultiTouchDevice::GetActiveTouchCount() const {
    int32_t count = 0;
    for (const TouchPoint& touch : m_Touches) {
        count += touch.IsActive() ? 1 : 0;
    }
    return count;
}

bool MultiTouchDevice::GetGesturePair(const TouchPoint*& OutA, const TouchPoint*& OutB) const {
    OutA = nullptr;
    OutB = nullptr;
    for (const TouchPoint& touch : m_Touches) {
        if (!touch.IsActive() || touch.BeganThisFrame) {
            continue;
        }
        if (!OutA) {
            OutA = &touch;
        } else {
            OutB = &touch;
            return true;
        }
    }
    return false;
}

float MultiTouchDevice::GetPinchDelta() const {
    const TouchPoint* a;
    const TouchPoint* b;
    if (!GetGesturePair(a, b)) {
        return 0.0f;
    }
    return glm::distance(a->Position, b->Position) - glm::distance(a->PreviousPosition, b->PreviousPosition);
}

float MultiTouchDevice::GetTwistDelta() const {
    const TouchPoint* a;
    const TouchPoint* b;
    if (!GetGesturePair(a, b)) {
        return 0.0f;
    }
    const Vec2 now = b->Position - a->Position;
    const Vec2 before = b->PreviousPosition - a->PreviousPosition;
    return WrapAngle(std::atan2(now.y, now.x) - std::atan2(before.y, before.x));
}

Vec2 MultiTouchDevice::GetPanDelta() const {
    const TouchPoint* a;
    const TouchPoint* b;
    if (!GetGesturePair(a, b)) {
        return Vec2(0.0f);
    }
    return (a->GetDelta() + b->GetDelta()) * 0.5f;
}

String MultiTouchDevice::GetDeviceName() const {
    return "Touchscreen";
}

Array<InputControl> MultiTouchDevice::GetControls() const {
    Array<InputControl> controls;
    controls.Add({"Primary", InputValueType::Bool});
    controls.Add({"PrimaryPosition", InputValueType::Vec2});
    controls.Add({"PrimaryDelta", InputValueType::Vec2});
    controls.Add({"Count", InputValueType::Float});
    controls.Add({"Pinch", InputValueType::Float});
    controls.Add({"Twist", InputValueType::Float});
    controls.Add({"Pan", InputValueType::Vec2});
    for (int32_t i = 0; i < AddressableTouches; i++) {
        const String name = s_TouchPrefix + std::to_string(i);
        controls.Add({name, InputValueType::Bool});
        controls.Add({name + "Position", InputValueType::Vec2});
        controls.Add({name + "Delta", InputValueType::Vec2});
    }
    return controls;
}

bool MultiTouchDevice::ReadControl(const String& InControl, InputValue& OutValue) {
    const TouchPoint* primary = GetPrimaryTouch();
    if (InControl == "Primary") {
        OutValue = MakeValue(primary && primary->IsActive() ? 1.0f : 0.0f);
        return true;
    }
    if (InControl == "PrimaryPosition") {
        OutValue = MakeValue(primary ? primary->Position : Vec2(0.0f));
        return true;
    }
    if (InControl == "PrimaryDelta") {
        OutValue = MakeValue(primary ? primary->GetDelta() : Vec2(0.0f));
        return true;
    }
    if (InControl == "Count") {
        OutValue = MakeValue((float)GetActiveTouchCount());
        return true;
    }
    if (InControl == "Pinch") {
        OutValue = MakeValue(GetPinchDelta());
        return true;
    }
    if (InControl == "Twist") {
        OutValue = MakeValue(GetTwistDelta());
        return true;
    }
    if (InControl == "Pan") {
        OutValue = MakeValue(GetPanDelta());
        return true;
    }
    return ReadIndexedControl(InControl, OutValue);
}

bool MultiTouchDevice::ReadIndexedControl(const String& InControl, InputValue& OutValue) const {
    const String prefix = s_TouchPrefix;
    if (InControl.rfind(prefix, 0) != 0) {
        return false;
    }

    size_t cursor = prefix.size();
    int32_t index = 0;
    const size_t digitsStart = cursor;
    while (cursor < InControl.size() && std::isdigit((unsigned char)InControl[cursor])) {
        index = index * 10 + (InControl[cursor] - '0');
        cursor++;
    }
    if (cursor == digitsStart || index >= AddressableTouches) {
        return false;
    }

    const TouchPoint* touch = index < m_Touches.Size() ? &m_Touches[index] : nullptr;
    const String suffix = InControl.substr(cursor);
    if (suffix.empty()) {
        OutValue = MakeValue(touch && touch->IsActive() ? 1.0f : 0.0f);
        return true;
    }
    if (suffix == "Position") {
        OutValue = MakeValue(touch ? touch->Position : Vec2(0.0f));
        return true;
    }
    if (suffix == "Delta") {
        OutValue = MakeValue(touch ? touch->GetDelta() : Vec2(0.0f));
        return true;
    }
    return false;
}

MultiTouchDevice* MultiTouchDevice::Instance() {
    for (const auto& It : InputSystem::Get().GetDevices()) {
        if (It->IsA<MultiTouchDevice>()) {
            return It->As<MultiTouchDevice>();
        }
    }
    return nullptr;
}

void MultiTouchDevice::Tick() {
    m_Touches.Filter([](const TouchPoint& InTouch) { return InTouch.IsActive(); });
    for (TouchPoint& touch : m_Touches) {
        touch.PreviousPosition = touch.Position;
        touch.Phase = TouchPhase::Stationary;
        touch.BeganThisFrame = false;
    }
    PollTouches();
}

TouchPoint* MultiTouchDevice::FindTouchMutable(uint64_t InId) {
    for (TouchPoint& touch : m_Touches) {
        if (touch.Id == InId && touch.IsActive()) {
            return &touch;
        }
    }
    return nullptr;
}

void MultiTouchDevice::BeginTouch(uint64_t InId, const Vec2& InPosition, float InPressure, float InRadius) {
    if (FindTouchMutable(InId)) {
        MoveTouch(InId, InPosition, InPressure, InRadius);
        return;
    }

    TouchPoint touch;
    touch.Id = InId;
    touch.Phase = TouchPhase::Began;
    touch.Position = InPosition;
    touch.PreviousPosition = InPosition;
    touch.StartPosition = InPosition;
    touch.Pressure = InPressure;
    touch.Radius = InRadius;
    touch.StartTime = Now();
    touch.BeganThisFrame = true;
    m_Touches.Add(touch);
}

void MultiTouchDevice::MoveTouch(uint64_t InId, const Vec2& InPosition, float InPressure, float InRadius) {
    TouchPoint* touch = FindTouchMutable(InId);
    if (!touch) {
        return;
    }
    touch->Position = InPosition;
    touch->Pressure = InPressure;
    touch->Radius = InRadius;
    if (touch->Phase != TouchPhase::Began) {
        touch->Phase = TouchPhase::Moved;
    }
}

void MultiTouchDevice::EndTouch(uint64_t InId, const Vec2& InPosition) {
    if (TouchPoint* touch = FindTouchMutable(InId)) {
        touch->Position = InPosition;
        touch->Phase = TouchPhase::Ended;
    }
}

void MultiTouchDevice::CancelTouch(uint64_t InId) {
    if (TouchPoint* touch = FindTouchMutable(InId)) {
        touch->Phase = TouchPhase::Cancelled;
    }
}

void MultiTouchDevice::CancelAllTouches() {
    for (TouchPoint& touch : m_Touches) {
        if (touch.IsActive()) {
            touch.Phase = TouchPhase::Cancelled;
        }
    }
}
