#pragma once

#include "core/WindowMetrics.h"
#include "core/components/Camera.h"
#include "core/components/Transform.h"
#include <glm/glm.hpp>

class CameraUtils
{
public:
    // Converts a captured screen position (where 0,0 is top-left)
    // to World Coordinates, accounting for the camera's aspect ratio, zoom, and position.
    static glm::vec2 screenToWorld(
        const WindowMetrics &window, const Camera &camera, const Transform &camTransform, glm::vec2 screenPos);
};
