#pragma once

#include <utility>

struct Viewport
{
    int x = 0, y = 0, width = 0, height = 0;
};

// Captured on the platform thread. All calculations use these frozen values.
struct WindowMetrics
{
    int width = 0;
    int height = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    float contentScaleX = 1.0f;
    float contentScaleY = 1.0f;

    std::pair<int, int> getWindowSize() const
    {
        return {width, height};
    }
    std::pair<int, int> getFrameBufferSize() const
    {
        return {framebufferWidth, framebufferHeight};
    }

    Viewport getViewportForAspect(float targetAspect) const
    {
        if (framebufferWidth <= 0 || framebufferHeight <= 0 || !(targetAspect > 0.0f))
        {
            return {};
        }
        const float windowAspect = static_cast<float>(framebufferWidth) / framebufferHeight;
        if (targetAspect > windowAspect)
        {
            const int viewportHeight = static_cast<int>(framebufferWidth / targetAspect);
            return {0, (framebufferHeight - viewportHeight) / 2, framebufferWidth, viewportHeight};
        }
        const int viewportWidth = static_cast<int>(framebufferHeight * targetAspect);
        return {(framebufferWidth - viewportWidth) / 2, 0, viewportWidth, framebufferHeight};
    }
};
