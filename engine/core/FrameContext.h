#pragma once

#include "core/InputSnapshot.h"
#include "core/WindowMetrics.h"

// One value owned by the frame loop and passed by const reference to all readers.
// deltaTime is still variable; fixed simulation ticks are a separate migration.
struct FrameContext
{
    float deltaTime = 0.0f;  // Seconds since the previous frame.
    double elapsedTime = 0.0; // Captured platform-clock seconds for presentation.
    InputSnapshot input;
    WindowMetrics window;
};

// Main thread only, immediately after polling platform events. Keep the returned
// value alive until all work reading this frame has completed.
FrameContext captureFrameContext(float deltaTime, double elapsedTime);
