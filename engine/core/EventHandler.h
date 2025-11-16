#pragma once

#include "core/GLIncludes.h"
#include "core/Event.h"

#include <map>
#include <queue>

// TODO: handle mouse events here as well
class EventHandler
{
public:
    static EventHandler *instance();

    bool isKeyPressed(KeyType key) const;

    bool hasPendingEvents() const;
    Event nextEvent();

    // The idea here is we have an event listener for other windowing
    // systems we can add later as well. Then our window would choose
    // which one to use.
    static void glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods);

private:
    EventHandler() = default;

    std::queue<Event> m_eventQueue;
    std::unordered_map<KeyType, bool> m_keyStates;
};