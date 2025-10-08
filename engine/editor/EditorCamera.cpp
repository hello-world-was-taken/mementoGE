#include "editor/EditorCamera.h"
#include "editor/Constants.h"

#include <glm/gtc/matrix_transform.hpp>

void EditorCamera::onViewportResize(float fbWidth, float fbHeight)
{
    float unitsPerPixelX = LOGICAL_WIDTH / WINDOW_WIDTH;
    float unitsPerPixelY = LOGICAL_HEIGHT / WINDOW_HEIGHT;

    float scaledWidth = unitsPerPixelX * fbWidth;
    float scaledHeight = unitsPerPixelY * fbHeight;

    // apply zoom
    scaledWidth *= m_zoom;
    scaledHeight *= m_zoom;

    m_projection = glm::ortho(
        0.0f, scaledWidth,
        0.0f, scaledHeight,
        -100.0f, 100.0f);
}

void EditorCamera::update(float deltaTime, const glm::vec2 &inputMovement)
{
    m_position += glm::vec3(inputMovement, 0.0f) * deltaTime;
    m_view = glm::translate(glm::mat4(1.0f), -glm::vec3(m_position));
}
