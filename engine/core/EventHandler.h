#pragma once

#include "core/GLIncludes.h"
#include "core/Event.h"

#include <map>

// TODO: handle mouse events here as well
class EventHandler
{
public:
    static EventHandler *get();

    bool hasActiveEvent();
    const Event getCurrentEvent();
    bool isKeyPressed(KeyType key) const;

    // The idea here is we have an event listener for other windowing
    // systems we can add later as well. Then our window would choose
    // which one to use.
    static void glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods);

private:
    EventHandler() = default;

    bool m_hasActiveEvent;

    // used for game logic with single-action triggers. E.g: when user clicks on ESC
    // to toggle settings/menu.
    Event m_currentEvent{"None", EventType::None, false, KeyType::None};
    std::unordered_map<KeyType, bool> m_keyStates;
};