#pragma once

#include "core/components/Camera.h"
#include "core/components/Transform.h"

#include <glm/glm.hpp>

// Editor navigation owns the same camera data used by scene entities.
class EditorCamera
{
public:
    EditorCamera();

    void onViewportResize(float fbWidth, float fbHeight);

    // input handling for panning/zoom
    void update(float deltaTime, const glm::vec2 &inputMovement);

    void setPosition(const glm::vec3 &position);
    glm::vec3 getPosition() const;

    void adjustZoom(float delta);
    float getZoom() const;

    const Camera &camera() const;
    const Transform &transform() const;

private:
    Camera m_camera;
    Transform m_transform;
};
