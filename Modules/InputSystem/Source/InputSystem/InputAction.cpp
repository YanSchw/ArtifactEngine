#include "InputAction.h"
#include "InputSystem.h"

// Magnitude past which a control counts as actuated (drives the press/release
// edges). 0.5 works for buttons (0/1) and as a deadzone-ish gate for axes.
static constexpr float k_ActuationThreshold = 0.5f;

bool InputAction::IsActive() const {
    return m_Value.Magnitude() > k_ActuationThreshold;
}

bool InputAction::WasActive() const {
    return m_ValueLastFrame.Magnitude() > k_ActuationThreshold;
}

bool InputAction::WasPerformedThisFrame() const {
    return IsActive() && !WasActive();
}

bool InputAction::WasReleasedThisFrame() const {
    return !IsActive() && WasActive();
}

void InputAction::BindPressed(Object* InOwner, ActionCallback InCallback) {
    m_OnPressed.Bind(InOwner, std::move(InCallback));
}

void InputAction::BindReleased(Object* InOwner, ActionCallback InCallback) {
    m_OnReleased.Bind(InOwner, std::move(InCallback));
}

void InputAction::BindHeld(Object* InOwner, ActionCallback InCallback, bool InIncludePressFrame) {
    if (InIncludePressFrame) {
        m_OnHeld.Bind(InOwner, std::move(InCallback));
    } else {
        m_OnHeldAfterPress.Bind(InOwner, std::move(InCallback));
    }
}

void InputAction::BindValue(Object* InOwner, ActionCallback InCallback, bool InContinuous) {
    if (InContinuous) {
        m_OnValueContinuous.Bind(InOwner, std::move(InCallback));
    } else {
        m_OnValueChanged.Bind(InOwner, std::move(InCallback));
    }
}

void InputAction::Unbind(Object* InOwner) {
    m_OnPressed.Unbind(InOwner);
    m_OnReleased.Unbind(InOwner);
    m_OnHeld.Unbind(InOwner);
    m_OnHeldAfterPress.Unbind(InOwner);
    m_OnValueChanged.Unbind(InOwner);
    m_OnValueContinuous.Unbind(InOwner);
}

void InputAction::Evaluate() {
    m_ValueLastFrame = m_Value;

    // Disambiguation: the strongest binding wins (e.g. WASD vs gamepad stick).
    InputValue best;
    float bestMagnitude = 0.0f;
    for (const auto& binding : Bindings) {
        InputValue value = binding->Read();
        float magnitude = value.Magnitude();
        if (magnitude > bestMagnitude) {
            bestMagnitude = magnitude;
            best = value;
        }
    }
    m_Value = best;

    // Remember which device drove us active, for GetLastActiveDevice().
    if (IsActive() && m_Value.Device) {
        InputSystem::Get().SetLastActiveDevice(m_Value.Device);
    }

    // Bool-style edges.
    if (WasPerformedThisFrame()) {
        m_OnPressed.Broadcast(m_Value);
    }
    if (WasReleasedThisFrame()) {
        m_OnReleased.Broadcast(m_Value);
    }
    if (IsActive()) {
        m_OnHeld.Broadcast(m_Value);
        if (!WasPerformedThisFrame()) {
            m_OnHeldAfterPress.Broadcast(m_Value);
        }
    }

    // Float / Vec2.
    if (m_Value.Raw != m_ValueLastFrame.Raw) {
        m_OnValueChanged.Broadcast(m_Value);  // including settling back to zero
    }
    if (m_Value.Raw != Vec2{0.0f, 0.0f}) {
        m_OnValueContinuous.Broadcast(m_Value);
    }
}
