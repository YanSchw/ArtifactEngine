#pragma once
#include <cstdint>

/** Standard pointer shapes a surface backend can display (GLFW maps these to the OS cursors). */
enum class CursorIcon : uint8_t {
    Arrow = 0,
    Text,        // I-beam, for editable/selectable text
    Hand,        // pointing hand, for clickable elements
    Crosshair,
    ResizeH,     // <-> horizontal resize
    ResizeV,     // vertical resize
    ResizeNWSE,  // diagonal resize, top-left <-> bottom-right
    ResizeNESW,  // diagonal resize, top-right <-> bottom-left
    ResizeAll,
    NotAllowed,
};
