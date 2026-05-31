#pragma once

#include <string>

enum class EventType
{
    None = 0,
    Key, // keyboard keys
    KeyRepeat,
    KeyRelease,
    MouseMove,
    MouseRightClick,
    MouseLeftClick,
    MouseButtonPress,
    MouseButtonRelease,
    MouseScroll,
    WindowResize,
    WindowMove
};

enum class KeyType
{
    None = 0,
    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,
    Escape,
    W,
    A,
    S,
    D,
    F,
    M,
    C,
    V,
    Enter,
    Z,
    Y,
    Cmd,
    LeftShift,
    RightShift,
    Space,
};

enum class MouseButton
{
    Left = 0,
    Right = 1,
    Middle = 2,
};

struct Event
{
    std::string name;
    EventType type = EventType::None; // keyboard, mouse, window events
    bool isPressed = false;
    KeyType keyType = KeyType::None;

    // modifiers, maybe we could use bit manipulation instead
    bool cmd = false;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    int mouseButton = -1;
};
