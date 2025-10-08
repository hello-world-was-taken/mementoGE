#include "core/Camera.h"
#include "editor/Constants.h"

#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

Camera::Camera()
{
    m_projection = glm::mat4(1.0f);
    m_position = glm::vec3(0.0f);
    m_view = glm::translate(glm::mat4(1.0f), -glm::vec3(m_position));

    // Note: projection is updated in Window::setUserData for initial projection update
}

// Camera::~Camera()
// {
// }

const glm::mat4 &Camera::getViewMatrix() const
{
    return m_view;
}

const glm::mat4 &Camera::getProjectionMatrix() const
{
    return m_projection;
}

void Camera::setPosition(const glm::vec3 &position)
{
    m_position = position;
    m_view = glm::translate(glm::mat4(1.0f), -glm::vec3(m_position));
}

glm::vec3 Camera::getPosition() const
{
    return m_position;
}

void Camera::adjustZoom(float delta)
{
    // Zoom sensitivity multiplier for smoothing out
    const float zoomSpeed = 0.1f;

    m_zoom -= delta * zoomSpeed;
    m_zoom = std::clamp(m_zoom, m_minZoom, m_maxZoom);
}

void Camera::setZoom(float zoom)
{
    m_zoom = zoom;
}

float Camera::getZoom() const
{
    return m_zoom;
}