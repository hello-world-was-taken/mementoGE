#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "engine/core/Camera.h"

Camera::Camera(float width, float height) : m_width(width), m_height(height)
{
    m_projection = glm::mat4(1.0f);
    m_view = glm::mat4(1.0f);
    m_position = glm::vec3(0.0f);

    updateView();
    updateProjection();
}

Camera::~Camera()
{
}

// TODO: How can we improve the view?
void Camera::updateView()
{
    glm::vec3 camera_front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
    m_view = glm::lookAt(
        glm::vec3(m_position, 10.0f),
        glm::vec3(m_position, 0.0f) + camera_front,
        camera_up);
}

void Camera::updateProjection()
{
    m_projection = glm::ortho(
        0.0f, m_width,
        0.0f, m_height, -100.0f, 100.0f); // -1 to 1 in depth (near and far planes)
}

glm::mat4 Camera::getViewMatrix() const
{
    return m_view;
}

glm::mat4 Camera::getProjectionMatrix() const
{
    return m_projection;
}

void Camera::update(float deltaTime, glm::vec2 translationVector)
{
    m_position = glm::vec3(m_position, 0.0f) + glm::vec3(translationVector, 0.0f);
    updateView();
}