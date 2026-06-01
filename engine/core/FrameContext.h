#pragma once

#include "core/InputSnapshot.h"
#include "core/WindowMetrics.h"

// One value owned by the frame loop and passed by const reference to all readers.
// Frame capture describes presentation time. The main run loops supply systems
// with a tick context whose timing advances only in fixed simulation steps.
struct FrameContext
{
    float deltaTime = 0.0f;   // Seconds: fixed step for systems, real frame delta for presentation.
    double elapsedTime = 0.0; // Simulation seconds for ticks, platform-clock seconds for presentation.
    InputSnapshot input;
    WindowMetrics window;
};

// Main thread only, immediately after polling platform events. Keep the returned
// value alive until all work reading this frame has completed.
FrameContext captureFrameContext(float deltaTime, double elapsedTime);
