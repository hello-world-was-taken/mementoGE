#pragma once

#include <string>

enum class EventType
{
    None = 0,
    Key, // keyboard keys
    KeyRepeat,
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
    A,
    S,
    D,
    W,
    SPACE,
    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,
    Escape,
};

class Event
{
public:
    Event(std::string name, EventType type, bool isPressed, KeyType keyType = KeyType::None);

    const std::string &getEventName() const;
    EventType getEventType() const; // keyboard, mouse, window events
    bool isKeyPressEvent() const;
    KeyType getKeyType() const;

private:
    std::string mEventName;
    EventType mEventType;
    bool mIsPressed;
    KeyType mKeyType;
};