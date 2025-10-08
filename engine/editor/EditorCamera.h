#pragma once

#include "core/Camera.h"

#include <glm/glm.hpp>

class EditorCamera : public Camera
{
public:
    EditorCamera() = default;

    void onViewportResize(float fbWidth, float fbHeight);

    // input handling for panning/zoom
    void update(float deltaTime, const glm::vec2 &inputMovement);
};
