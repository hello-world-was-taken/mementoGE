#pragma once

#include <string>

enum class EventType
{
    None = 0,
    KeyPress,
    KeyRepeat,
    KeyRelease,
    MouseMove,
    MouseRightClick,
    MouseLeftClick,
    MouseButtonPress,
    MouseButtonRelease,
    MouseScroll,
    WindowResize,
    WindowMove,
    KeyHeld,
    MouseButtonHeld,
    MousePosition,
};

// Engine key identities. Platform backends translate their own native codes.
enum class KeyType
{
    None,
    Unknown,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    Space,
    Apostrophe,
    Comma,
    Minus,
    Period,
    Slash,
    Semicolon,
    Equal,
    LeftBracket,
    Backslash,
    RightBracket,
    GraveAccent,
    International1,
    International2,
    Escape,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    F25,
    Keypad0,
    Keypad1,
    Keypad2,
    Keypad3,
    Keypad4,
    Keypad5,
    Keypad6,
    Keypad7,
    Keypad8,
    Keypad9,
    KeypadDecimal,
    KeypadDivide,
    KeypadMultiply,
    KeypadSubtract,
    KeypadAdd,
    KeypadEnter,
    KeypadEqual,
    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,
    LeftSuper,
    RightSuper,
    Menu,
};

enum class MouseButton
{
    None,
    Unknown,
    Left,
    Right,
    Middle,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,
};

struct Event
{
    std::string name;
    EventType type = EventType::None; // keyboard, mouse, window events
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
    MouseButton mouseButton = MouseButton::None;

    // Window-coordinate motion, with positive delta Y pointing upward.
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;
    bool insideSceneViewport = true;
};
