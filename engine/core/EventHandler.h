#pragma once

#include <GLFW/glfw3.h>

#include "core/Event.h"

class EventHandler
{
public:
    EventHandler();
    ~EventHandler();

    static bool hasActiveEvent();
    // TODO: can we improve this to not call copy
    // constructor?
    static Event getCurrentEvent();
    // the idea here is we have an event listener for other windowing
    // systems we can add later as well. Then our window would choose
    // which one to use.
    static void glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods);

    static bool mHasActiveEvent;
    static Event mCurrentEvent;
};