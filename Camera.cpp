#include "Camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/ext/matrix_clip_space.hpp"


Camera::Camera(float width, float height) : m_width(width), m_height(height) {
    m_projection = glm::mat4(1.0f);
    m_view = glm::mat4(1.0f);
    m_position = glm::vec2(0.0f);

    updateView();
    updateProjection();
}


Camera::~Camera() {
}


// TODO: How can we improve the view?
void Camera::updateView() {
    m_view = glm::translate(glm::mat3(1.0f), -glm::vec2(m_position.x, m_position.y));
}


void Camera::updateProjection() {
    m_projection = glm::ortho(
            0.0f, m_width, 
            0.0f, m_height, -1.0f, 1.0f
        ); // -1 to 1 in depth (near and far planes)
}


glm::mat4 Camera::getViewMatrix() const {
    return m_view;
}


glm::mat4 Camera::getProjectionMatrix() const {
    return m_projection;
}