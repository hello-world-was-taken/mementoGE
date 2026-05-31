#pragma once

class Time
{
private:
    static double m_last_frame_time;
    static float m_delta_time; // Delta time in seconds

public:
    Time();
    ~Time();

    // Platform-thread clock capture. Update work reads FrameContext instead.
    static void reset();
    static void update();
    static float deltaTime();
    static double elapsedTime();
};
