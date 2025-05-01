#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "core/Camera.h"

Camera::Camera(float width, float height) : m_width(width), m_height(height)
{
    m_projection = glm::mat4(1.0f);
    m_view = glm::mat4(1.0f);
    m_position = glm::vec3(0.0f);

    updateView();
    // TODO: this is not using the framebuffer when we
    // instantiate it in the scene
    updateProjection(width, height);
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

// using virtual aspect ratio to avoid squishing and stretching
void Camera::updateProjection(float fbWidth, float fbHeight)
{
    float aspectFramebuffer = fbWidth / fbHeight;
    float aspectVirtual = m_width / m_height;

    float newWidth = m_width;
    float newHeight = m_height;

    if (aspectFramebuffer > aspectVirtual)
    {
        // Framebuffer is wider than virtual
        newWidth = m_height * aspectFramebuffer;
    }
    else
    {
        // Framebuffer is taller than virtual
        newHeight = m_width / aspectFramebuffer;
    }

    m_projection = glm::ortho(
        0.0f, newWidth,
        0.0f, newHeight,
        -100.0f, 100.0f);
}

void Camera::onWindowResize(int framebufferWidth, int framebufferHeight)
{
    updateProjection(static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));
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