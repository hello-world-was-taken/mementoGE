#include "editor/EditorCamera.h"
#include "core/CameraUtils.h"
#include "editor/Constants.h"

#include <algorithm>

EditorCamera::EditorCamera()
{
    m_camera.primary = false;
    m_transform.position = glm::vec3(0.0f);

    CameraUtils::updateMatrices(m_camera, m_transform);
}

void EditorCamera::onViewportResize(float fbWidth, float fbHeight)
{
    m_camera.logicalWidth = (LOGICAL_WIDTH / WINDOW_WIDTH) * fbWidth;
    m_camera.logicalHeight = (LOGICAL_HEIGHT / WINDOW_HEIGHT) * fbHeight;

    CameraUtils::updateMatrices(m_camera, m_transform);
}

void EditorCamera::update(float deltaTime, const glm::vec2 &inputMovement)
{
    setPosition(m_transform.position + glm::vec3(inputMovement, 0.0f) * deltaTime);
}

void EditorCamera::setPosition(const glm::vec3 &position)
{
    m_transform.position = position;

    CameraUtils::updateMatrices(m_camera, m_transform);
}

glm::vec3 EditorCamera::getPosition() const
{
    return m_transform.position;
}

void EditorCamera::adjustZoom(float delta)
{
    constexpr float zoomSpeed = 0.1f;
    m_camera.zoom = std::clamp(m_camera.zoom - delta * zoomSpeed, m_camera.minZoom, m_camera.maxZoom);

    CameraUtils::updateMatrices(m_camera, m_transform);
}

float EditorCamera::getZoom() const
{
    return m_camera.zoom;
}

const Camera &EditorCamera::camera() const
{
    return m_camera;
}

const Transform &EditorCamera::transform() const
{
    return m_transform;
}
