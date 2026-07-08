#pragma once
#include "Common/Types.h"

/** Cursor: drives a pointer over Interactable nodes
 *  Focus: keeps one node focused and moves it with Nav* inputs. */
enum class UIInputMode : uint8_t { Cursor = 0, Focus = 1 };

enum class UINavDirection : uint8_t { Up = 0, Down = 1, Left = 2, Right = 3 };

/** Per-frame input handed to UICanvas::RunFrame. */
struct UIFrameContext {
    Vec2 CursorPosition = Vec2(0.0f); // screen pixels, top-left origin
    bool CursorDown = false;
    bool CursorPressedThisFrame = false;
    bool CursorReleasedThisFrame = false;
    Vec2 ScrollDelta = Vec2(0.0f);

    bool NavUp = false;
    bool NavDown = false;
    bool NavLeft = false;
    bool NavRight = false;
    bool NavSelectPressed = false;
    bool NavSelectReleased = false;
    bool NavBack = false;

    float DeltaTime = 0.0f;
};
