#include "Time.h"

Time::Time() {}
Time::~Time() {}

float Time::m_last_frame_time = glfwGetTime();

float Time::m_delta_time = 0.0f;

// Update the current time and calculate delta time
void Time::update()
{
    float current_frame_time = glfwGetTime();
    m_delta_time = current_frame_time - m_last_frame_time;
    m_last_frame_time = current_frame_time;
}

float Time::deltaTime()
{
    return m_delta_time;
}
