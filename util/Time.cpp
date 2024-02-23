#include <chrono>
#include "./Time.h"


Time::Time(){}
Time::~Time(){}


std::chrono::time_point<std::chrono::high_resolution_clock> Time::m_last_frame_time = std::chrono::high_resolution_clock::now();

float Time::m_delta_time = 0.0f;


// Update the current time and calculate delta time
void Time::update() {
    auto current_frame_time = std::chrono::high_resolution_clock::now();
    m_delta_time = std::chrono::duration<float>(current_frame_time - m_last_frame_time).count();  //count gives us second
    m_last_frame_time = current_frame_time;
}


float Time::deltaTime() {
    return m_delta_time;
}
