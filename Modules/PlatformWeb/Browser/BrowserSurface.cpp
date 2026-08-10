#include "BrowserSurface.h"

#include "BrowserGamepadDevice.h"
#include "BrowserKeyboardDevice.h"
#include "BrowserMouseDevice.h"
#include "InputSystem/InputSystem.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <unordered_map>

static constexpr const char* s_Canvas = "#canvas";
static BrowserSurface* s_Instance = nullptr;

static const std::unordered_map<std::string, KeyCode>& GetKeyTable() {
    static const std::unordered_map<std::string, KeyCode> table = [] {
        std::unordered_map<std::string, KeyCode> keys = {
            { "Space", KeyCode::Space },       { "Quote", KeyCode::Apostrophe },
            { "Comma", KeyCode::Comma },       { "Minus", KeyCode::Minus },
            { "Period", KeyCode::Period },     { "Slash", KeyCode::Slash },
            { "Semicolon", KeyCode::Semicolon }, { "Equal", KeyCode::Equal },
            { "BracketLeft", KeyCode::LeftBracket }, { "Backslash", KeyCode::Backslash },
            { "BracketRight", KeyCode::RightBracket }, { "Backquote", KeyCode::GraveAccent },
            { "Escape", KeyCode::Escape },     { "Enter", KeyCode::Enter },
            { "Tab", KeyCode::Tab },           { "Backspace", KeyCode::Backspace },
            { "Insert", KeyCode::Insert },     { "Delete", KeyCode::Delete },
            { "ArrowRight", KeyCode::Right },  { "ArrowLeft", KeyCode::Left },
            { "ArrowDown", KeyCode::Down },    { "ArrowUp", KeyCode::Up },
            { "PageUp", KeyCode::PageUp },     { "PageDown", KeyCode::PageDown },
            { "Home", KeyCode::Home },         { "End", KeyCode::End },
            { "CapsLock", KeyCode::CapsLock }, { "ScrollLock", KeyCode::ScrollLock },
            { "NumLock", KeyCode::NumLock },   { "PrintScreen", KeyCode::PrintScreen },
            { "Pause", KeyCode::Pause },       { "ContextMenu", KeyCode::Menu },
            { "ShiftLeft", KeyCode::LeftShift }, { "ControlLeft", KeyCode::LeftControl },
            { "AltLeft", KeyCode::LeftAlt },   { "MetaLeft", KeyCode::LeftSuper },
            { "ShiftRight", KeyCode::RightShift }, { "ControlRight", KeyCode::RightControl },
            { "AltRight", KeyCode::RightAlt }, { "MetaRight", KeyCode::RightSuper },
            { "NumpadDecimal", KeyCode::KPDecimal }, { "NumpadDivide", KeyCode::KPDivide },
            { "NumpadMultiply", KeyCode::KPMultiply }, { "NumpadSubtract", KeyCode::KPSubtract },
            { "NumpadAdd", KeyCode::KPAdd },   { "NumpadEnter", KeyCode::KPEnter },
            { "NumpadEqual", KeyCode::KPEqual },
        };

        for (int32_t i = 0; i < 26; i++) {
            keys[std::string("Key") + (char)('A' + i)] = (KeyCode)((uint16_t)KeyCode::A + i);
        }
        for (int32_t i = 0; i < 10; i++) {
            const char digit = (char)('0' + i);
            keys[std::string("Digit") + digit] = (KeyCode)((uint16_t)KeyCode::D0 + i);
            keys[std::string("Numpad") + digit] = (KeyCode)((uint16_t)KeyCode::KP0 + i);
        }
        for (int32_t i = 0; i < 25; i++) {
            keys["F" + std::to_string(i + 1)] = (KeyCode)((uint16_t)KeyCode::F1 + i);
        }
        return keys;
    }();
    return table;
}

// The browser numbers the middle and right buttons the other way round.
static MouseCode ToMouseCode(int32_t InBrowserButton) {
    switch (InBrowserButton) {
        case 1:  return MouseCode::Middle;
        case 2:  return MouseCode::Right;
        default: return (MouseCode)InBrowserButton;
    }
}

static float GetDevicePixelRatio() {
    return (float)emscripten_get_device_pixel_ratio();
}

// A wheel notch is ~100px; line and page deltas are already coarse. Positive scrolls up, as GLFW.
static Vec2 NormalizeWheel(const EmscriptenWheelEvent* InEvent) {
    const float scale = InEvent->deltaMode == DOM_DELTA_PIXEL ? 0.01f : 1.0f;
    return Vec2(-(float)InEvent->deltaX, -(float)InEvent->deltaY) * scale;
}

static EM_BOOL OnKeyEvent(int InEventType, const EmscriptenKeyboardEvent* InEvent, void* InUserData) {
    BrowserSurface* surface = (BrowserSurface*)InUserData;
    surface->OnKeyChanged(InEvent->code, InEventType == EMSCRIPTEN_EVENT_KEYDOWN);
    return GetKeyTable().count(InEvent->code) > 0;
}

static EM_BOOL OnMouseEvent(int InEventType, const EmscriptenMouseEvent* InEvent, void* InUserData) {
    BrowserSurface* surface = (BrowserSurface*)InUserData;
    const float ratio = GetDevicePixelRatio();

    switch (InEventType) {
        case EMSCRIPTEN_EVENT_MOUSEMOVE:
            surface->OnMouseMoved(Vec2((float)InEvent->targetX, (float)InEvent->targetY) * ratio,
                                  Vec2((float)InEvent->movementX, (float)InEvent->movementY) * ratio);
            break;
        case EMSCRIPTEN_EVENT_MOUSEDOWN:
            surface->OnMouseButtonChanged(InEvent->button, true);
            break;
        default:
            surface->OnMouseButtonChanged(InEvent->button, false);
            break;
    }
    return EM_TRUE;
}

static EM_BOOL OnWheelEvent(int, const EmscriptenWheelEvent* InEvent, void* InUserData) {
    ((BrowserSurface*)InUserData)->OnScrolled(NormalizeWheel(InEvent));
    return EM_TRUE;
}

static EM_BOOL OnPointerLockEvent(int, const EmscriptenPointerlockChangeEvent* InEvent, void* InUserData) {
    ((BrowserSurface*)InUserData)->OnPointerLockChanged(InEvent->isActive);
    return EM_TRUE;
}

void BrowserSurface::Initialize(const SurfaceParams& InParams) {
    s_Instance = this;
    emscripten_set_window_title(InParams.Title.c_str());

    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes);
    attributes.majorVersion = 2;
    attributes.minorVersion = 0;
    attributes.alpha = EM_FALSE;
    attributes.depth = EM_FALSE;
    attributes.stencil = EM_FALSE;
    // The scene resolves its own multisampling into the drawing buffer, which is only ever blitted.
    attributes.antialias = EM_FALSE;
    attributes.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;

    m_Context = emscripten_webgl_create_context(s_Canvas, &attributes);
    if (m_Context <= 0) {
        AE_ERROR("WebGL 2 is not available in this browser");
        return;
    }
    emscripten_webgl_make_context_current(m_Context);
    ResizeToCanvas();

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_TRUE, OnKeyEvent);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_TRUE, OnKeyEvent);
    emscripten_set_mousemove_callback(s_Canvas, this, EM_TRUE, OnMouseEvent);
    emscripten_set_mousedown_callback(s_Canvas, this, EM_TRUE, OnMouseEvent);
    // Releases outside the canvas still have to clear the button, so they come off the window.
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_TRUE, OnMouseEvent);
    emscripten_set_wheel_callback(s_Canvas, this, EM_TRUE, OnWheelEvent);
    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, EM_TRUE, OnPointerLockEvent);

    InputSystem::Get().AddDevice(new BrowserKeyboardDevice());
    InputSystem::Get().AddDevice(new BrowserMouseDevice());
    InputSystem::Get().AddDevice(new BrowserGamepadDevice());
}

BrowserSurface::~BrowserSurface() {
    if (s_Instance == this) {
        s_Instance = nullptr;
    }
    if (m_Context > 0) {
        emscripten_webgl_destroy_context(m_Context);
        m_Context = 0;
    }
}

BrowserSurface* BrowserSurface::Get() {
    return s_Instance;
}

void BrowserSurface::ResizeToCanvas() {
    double cssWidth = 0.0;
    double cssHeight = 0.0;
    emscripten_get_element_css_size(s_Canvas, &cssWidth, &cssHeight);

    const float ratio = GetDevicePixelRatio();
    const uint32_t width = (uint32_t)glm::max(cssWidth * ratio, 1.0);
    const uint32_t height = (uint32_t)glm::max(cssHeight * ratio, 1.0);
    if (width == m_Width && height == m_Height) {
        return;
    }

    m_Width = width;
    m_Height = height;
    emscripten_set_canvas_element_size(s_Canvas, (int)width, (int)height);
}

void BrowserSurface::ProcessEvents() {
    ResizeToCanvas();
}

void BrowserSurface::SetCursorLocked(bool InLocked) {
    m_CursorLockRequested = InLocked;
    if (InLocked) {
        // Browsers only grant this from a user gesture; a refused request is retried on the next click.
        emscripten_request_pointerlock(s_Canvas, EM_TRUE);
    } else {
        emscripten_exit_pointerlock();
    }
}

void BrowserSurface::OnPointerLockChanged(bool InLocked) {
    m_CursorLocked = InLocked;
}

bool BrowserSurface::IsKeyPressed(KeyCode InKey) const {
    return m_Keys.ContainsKey(InKey) && m_Keys.At(InKey);
}

bool BrowserSurface::IsMouseButtonPressed(MouseCode InButton) const {
    return m_Buttons.ContainsKey(InButton) && m_Buttons.At(InButton);
}

Vec2 BrowserSurface::ConsumeScrollDelta() {
    const Vec2 delta = m_ScrollAccum;
    m_ScrollAccum = Vec2(0.0f);
    return delta;
}

void BrowserSurface::OnKeyChanged(const String& InCode, bool InPressed) {
    const auto& table = GetKeyTable();
    const auto entry = table.find(InCode);
    if (entry != table.end()) {
        m_Keys[entry->second] = InPressed;
    }
}

void BrowserSurface::OnMouseButtonChanged(int32_t InBrowserButton, bool InPressed) {
    m_Buttons[ToMouseCode(InBrowserButton)] = InPressed;

    if (InPressed && m_CursorLockRequested && !m_CursorLocked) {
        emscripten_request_pointerlock(s_Canvas, EM_TRUE);
    }
}

void BrowserSurface::OnMouseMoved(const Vec2& InCanvasPosition, const Vec2& InMovement) {
    // A locked pointer stops reporting a position, so the engine's cursor keeps accumulating
    // movement instead - the same virtual cursor GLFW exposes for a disabled cursor.
    m_CursorPosition = m_CursorLocked ? m_CursorPosition + InMovement : InCanvasPosition;
}

void BrowserSurface::OnScrolled(const Vec2& InDelta) {
    m_ScrollAccum += InDelta;
}
