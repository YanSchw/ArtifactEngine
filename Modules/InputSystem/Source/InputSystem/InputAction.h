#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "InputValue.h"
#include "InputBinding.h"
#include "InputDelegate.h"
#include <functional>
#include "InputAction.gen.h"

class InputAction : public Object {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    String Name;

    PROPERTY()
    InputValueType Type = InputValueType::Bool;

    PROPERTY()
    Array<SharedObjectPtr<InputBinding>> Bindings;

    // Polling API
    bool  ReadBool()  const { return m_Value.AsBool(); }
    float ReadFloat() const { return m_Value.AsFloat(); }
    Vec2  ReadVec2()  const { return m_Value.AsVec2(); }
    InputValue Value() const { return m_Value; }

    bool IsActive() const;
    bool WasPerformedThisFrame() const;  // inactive -> active this frame
    bool WasReleasedThisFrame() const;   // active -> inactive this frame

    // Callback API. Each listener lives only as long as InOwner:
    // when that Object dies its callbacks are
    // pruned automatically, so no manual Unbind is required on teardown.
    //   Bool actions:         Pressed / Released / Held
    //   Float & Vec2 actions: Value
    using ActionCallback = std::function<void(const InputValue&)>;
    void BindPressed(Object* InOwner, ActionCallback InCallback);   // became active
    void BindReleased(Object* InOwner, ActionCallback InCallback);  // became inactive

    // Held: fires every frame while active. With InIncludePressFrame the very
    // first (press) frame is included; pass false to start the frame after.
    void BindHeld(Object* InOwner, ActionCallback InCallback, bool InIncludePressFrame = true);

    // Value: with InContinuous = false (default) fires only when the value
    // changes, including the frame it settles back to zero. With true it fires
    // every frame the value is non-zero instead.
    void BindValue(Object* InOwner, ActionCallback InCallback, bool InContinuous = false);

    void Unbind(Object* InOwner);  // remove every callback owned by InOwner

    // Evaluated once per frame by the owning action map.
    void Evaluate();

private:
    bool WasActive() const;

    InputValue m_Value;
    InputValue m_ValueLastFrame;

    InputDelegate<const InputValue&> m_OnPressed;
    InputDelegate<const InputValue&> m_OnReleased;
    InputDelegate<const InputValue&> m_OnHeld;            // includes the press frame
    InputDelegate<const InputValue&> m_OnHeldAfterPress;  // skips the press frame
    InputDelegate<const InputValue&> m_OnValueChanged;    // only when the value moves
    InputDelegate<const InputValue&> m_OnValueContinuous; // every frame while non-zero
};
