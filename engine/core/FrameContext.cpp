#include "core/FrameContext.h"
#include "core/EventHandler.h"
#include "core/Window.h"

FrameContext captureFrameContext(float deltaTime, double elapsedTime)
{
    FrameContext result;
    result.deltaTime = deltaTime;
    result.elapsedTime = elapsedTime;
    result.input = EventHandler::instance()->captureSnapshot();
    result.window = Window::instance()->captureMetrics();
    return result;
}
