#include "Time.h"
#include "core/Window.h"

Time::Time()
{
}
Time::~Time()
{
}

double Time::m_last_frame_time = 0.0;

float Time::m_delta_time = 0.0f;

// Update the current time and calculate delta time
void Time::reset()
{
    m_last_frame_time = Window::getWindowTime();
    m_delta_time = 0.0f;
}

void Time::update()
{
    const double current_frame_time = Window::getWindowTime();
    m_delta_time = static_cast<float>(current_frame_time - m_last_frame_time);
    m_last_frame_time = current_frame_time;
}

float Time::deltaTime()
{
    return m_delta_time;
}

double Time::elapsedTime()
{
    return m_last_frame_time;
}
