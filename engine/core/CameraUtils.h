#pragma once

#include "core/Window.h"
#include "core/components/Camera.h"
#include "core/components/Transform.h"
#include <glm/glm.hpp>

class CameraUtils
{
public:
    // Converts a screen position (from EventHandler::getMousePos, where 0,0 is top-left)
    // to World Coordinates, accounting for the camera's aspect ratio, zoom, and position.
    static glm::vec2 screenToWorld(
        const Window &window, const Camera &camera, const Transform &camTransform, glm::vec2 screenPos);
};
