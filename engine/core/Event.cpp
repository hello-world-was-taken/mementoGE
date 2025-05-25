#include "core/Event.h"

#include <string>

Event::Event(std::string name, EventType type, bool isPressed, KeyType keyType)
    : mEventName{name},
      mEventType{type},
      mIsPressed{isPressed},
      mKeyType{keyType}
{
}

const std::string &Event::getEventName() const
{
    return mEventName;
}

EventType Event::getEventType() const
{
    return mEventType;
}

bool Event::isKeyPressEvent() const
{
    return mIsPressed;
}

KeyType Event::getKeyType() const
{
    return mKeyType;
}