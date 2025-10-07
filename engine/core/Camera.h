
#pragma once

#include <glm/glm.hpp>


class Camera
{
public:
    Camera();
    virtual ~Camera() = default;

    // Movement / rotation
    void setPosition(const glm::vec3 &position);
    void setRotation(const glm::vec3 &rotation);
    void move(const glm::vec3 &delta);
    void rotate(const glm::vec3 &delta);

    // View / projection
    const glm::mat4 &getViewMatrix() const;
    const glm::mat4 &getProjectionMatrix() const;

    // Zoom
    void adjustZoom(float delta);
    float getZoom() const;
    void setZoom(float zoom);

    glm::vec3 getPosition() const;
    glm::vec3 getRotation() const;

protected:
    glm::mat4 m_projection{1.0f};
    glm::mat4 m_view{1.0f};

    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::vec3 m_rotation{0.0f};

    float m_zoom = 1.0f;

    // TODO: should these be in Constants.h?
    float m_minZoom = 0.1f;
    float m_maxZoom = 10.0f;
};
