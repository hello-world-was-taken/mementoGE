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
    Z,
    Y,
    Cmd,
    Space,
};

struct Event
{
    std::string name;
    EventType type; // keyboard, mouse, window events
    bool isPressed;
    KeyType keyType = KeyType::None;

    // modifiers, maybe we could use bit manipulation instead
    bool cmd = false;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};