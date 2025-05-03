#pragma once

#include <glm/glm.hpp>

#include "core/Camera.h"


class Camera {
public:
    Camera(float width, float height);
    ~Camera();

    void update(float deltaTime, glm::vec2 translationVector);
    void setProjectionMatrix(float fov, float aspect_ratio, float near_plane, float far_plane);
    void setPosition(glm::vec2 position);
    void setRotation(glm::vec3 rotation);

    void move(glm::vec3 direction);
    void rotate(glm::vec3 rotation);
    void onWindowResize(int framebufferWidth, int framebufferHeight);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::vec3 getPosition();
    glm::vec3 getRotation();

    void adjustZoom(float delta);
    float getZoom() const;
    void setZoom(float zoom);

    void updateProjection(float newWidth, float newHeight);

private:
    float m_width;
    float m_height;

    glm::mat4 m_projection;
    glm::mat4 m_view;
    glm::vec2 m_position; // 2D camera position

    float m_zoom = 1.0f;
    const float m_minZoom = 0.1f;
    const float m_maxZoom = 10.0f;

    void updateView();
};