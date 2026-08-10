#pragma once
#include "Rendering/Surface.h"
#include "InputSystem/KeyCodes.h"
#include "InputSystem/MouseCodes.h"
#include "BrowserSurface.gen.h"

/** The browser canvas as a render surface. It owns the WebGL 2 context and turns DOM events into
 *  the state the browser input devices report. */
class BrowserSurface : public Surface {
public:
    ARTIFACT_CLASS();

    virtual ~BrowserSurface();

    virtual void Initialize(const SurfaceParams& InParams) override;

    virtual uint32_t GetWidth() const override { return m_Width; }
    virtual uint32_t GetHeight() const override { return m_Height; }

    /** The canvas follows its CSS box, so a frame starts by matching the drawing buffer to it. */
    virtual void ProcessEvents() override;

    virtual void SetCursorLocked(bool InLocked) override;
    virtual bool IsCursorLocked() const override { return m_CursorLocked; }

    static BrowserSurface* Get();

    bool IsKeyPressed(KeyCode InKey) const;
    bool IsMouseButtonPressed(MouseCode InButton) const;
    Vec2 GetCursorPosition() const { return m_CursorPosition; }
    Vec2 ConsumeScrollDelta();

    void OnKeyChanged(const String& InCode, bool InPressed);
    void OnMouseButtonChanged(int32_t InBrowserButton, bool InPressed);
    void OnMouseMoved(const Vec2& InCanvasPosition, const Vec2& InMovement);
    void OnScrolled(const Vec2& InDelta);
    void OnPointerLockChanged(bool InLocked);

private:
    void ResizeToCanvas();

    uint32_t m_Width = 1;
    uint32_t m_Height = 1;
    Map<KeyCode, bool> m_Keys;
    Map<MouseCode, bool> m_Buttons;
    Vec2 m_CursorPosition = Vec2(0.0f);
    Vec2 m_ScrollAccum = Vec2(0.0f);
    bool m_CursorLocked = false;
    bool m_CursorLockRequested = false;
    int32_t m_Context = 0;
};
