#pragma once

#include <GLFW/glfw3.h>

#include "core/Event.h"

// TODO: handle mouse events here as well
class EventHandler
{
public:
    static EventHandler *get();

    bool hasActiveEvent();
    // TODO: can we improve this to not call copy
    // constructor?
    Event getCurrentEvent();
    // the idea here is we have an event listener for other windowing
    // systems we can add later as well. Then our window would choose
    // which one to use.
    static void glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods);

private:
    EventHandler() = default;

    bool m_hasActiveEvent;
    Event m_currentEvent{"", EventType::None, false};
};