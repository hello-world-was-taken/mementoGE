#include <chrono>

class Time {
private:
    static std::chrono::time_point<std::chrono::high_resolution_clock> m_last_frame_time;
    static float m_delta_time; // Delta time in seconds

public:
    Time();
    ~Time();

    static void update();
    static float deltaTime();
};