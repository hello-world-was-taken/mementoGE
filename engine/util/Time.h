#pragma once

#include <GLFW/glfw3.h>

class Time {
private:
    static float m_last_frame_time;
    static float m_delta_time; // Delta time in seconds

public:
    Time();
    ~Time();

    static void update();
    static float deltaTime();
};