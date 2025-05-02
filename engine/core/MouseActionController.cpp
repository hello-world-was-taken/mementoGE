#include <iostream>
#include <cmath>
#include <imgui.h>
#include <GLFW/glfw3.h>

#include "core/MouseActionController.h"
#include "core/Transform.h"
#include "util/Time.h"

MouseActionController::MouseActionController()
    : m_activeObject(nullptr) {}

void MouseActionController::SetActiveObject(GameObject &object)
{
    m_activeObject = &object;
}

void MouseActionController::Update(std::shared_ptr<Camera> camera, std::vector<GameObject> &gameObjects, ImVec2 imagePos, ImVec2 imageSize, int framebufferWidth, int framebufferHeight, GLFWwindow *window)
{
    MouseListener *mouse = MouseListener::get();
    glm::vec2 mouseWorldPos = getWorldCoordinate(camera, imagePos, imageSize, framebufferWidth, framebufferHeight);
    if (!m_activeObject)
    {
        if (mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
        {
            for (GameObject &obj : gameObjects)
            {
                if (obj.containsPoint(mouseWorldPos))
                {
                    SetActiveObject(obj);
                    break;
                }
            }
        }
    } else
    {
        // Assume grid size is equal to the object's width.
        // TODO: is there a scenario where we would want to change this?
        float gridSize = static_cast<float>(m_activeObject->getWidth());

        // Snap position: round down to nearest multiple of gridSize
        float snappedX = std::floor(mouseWorldPos.x / gridSize) * gridSize;
        float snappedY = std::floor(mouseWorldPos.y / gridSize) * gridSize;

        m_activeObject->getComponent<Transform>().setPosition(snappedX, snappedY, 0.0f);
    }

    bool draggingOnEmptySpace = mouse->isMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT) && !m_activeObject;
    if (draggingOnEmptySpace)
    {
        // Scale screen (window) coordinates to framebuffer space
        int winWidth, winHeight;
        glfwGetWindowSize(window, &winWidth, &winHeight);

        float scaleX = static_cast<float>(framebufferWidth) / winWidth;
        float scaleY = static_cast<float>(framebufferHeight) / winHeight;

        glm::vec2 dragDelta = mouse->getMouseDelta();

        camera->setPosition(camera->getPosition() - glm::vec3(dragDelta.x * scaleX, dragDelta.y * scaleY, 0.0f));
    }

    bool leftButtonClicked = mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || mouse->isMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT);

    if (!leftButtonClicked)
    {
        m_activeObject = nullptr;
    }
}

glm::vec2 MouseActionController::getWorldCoordinate(std::shared_ptr<Camera> camera, ImVec2 imagePos, ImVec2 imageSize, int framebufferWidth, int framebufferHeight)
{
    MouseListener *listener = MouseListener::get();

    // Mouse position in screen coordinates
    glm::vec2 mousePos = listener->getMouseScreenPosition();
    glm::vec2 localPos = screenToLocal(mousePos, {imagePos.x, imagePos.y}, {imageSize.x, imageSize.y});
    glm::vec2 fbPos = localToFrameBuffer(localPos, {imageSize.x, imageSize. y}, framebufferWidth, framebufferHeight);

    // Make sure the mouse is inside the image
    if (localPos.x < 0 || localPos.y < 0 || localPos.x > imageSize.x || localPos.y > imageSize.y)
        return glm::vec2(-1.0f);

    return frameBufferToWorld(camera, fbPos, framebufferWidth, framebufferHeight);
}

glm::vec2 MouseActionController::screenToLocal(glm::vec2 mousePos, glm::vec2 imagePos, glm::vec2 imageSize)
{
    // Get position of mouse relative to imgui scene preview window
    float localX = mousePos.x - imagePos.x;
    float localY = mousePos.y - imagePos.y;

    // Flip Y because ImGui has origin at top-left, OpenGL at bottom-left
    localY = imageSize.y - localY;

    return {localX, localY};
}

glm::vec2 MouseActionController::localToFrameBuffer(glm::vec2 localPos, glm::vec2 imageSize, int framebufferWidth, int framebufferHeight)
{
    float fbX = (localPos.x / imageSize.x) * framebufferWidth;
    float fbY = (localPos.y / imageSize.y) * framebufferHeight;

    return {fbX, fbY};
}

glm::vec2 MouseActionController::frameBufferToWorld(std::shared_ptr<Camera> camera, glm::vec2 fbPos, int framebufferWidth, int framebufferHeight)
{
    // Convert to Normalized Device Coordinates (NDC)
    float ndcX = (fbPos.x / framebufferWidth) * 2.0f - 1.0f;
    float ndcY = (fbPos.y / framebufferHeight) * 2.0f - 1.0f;

    // TODO: do we need to update z to layer images over one another?
    glm::vec4 clipCoords = glm::vec4(ndcX, ndcY, 0.0f, 1.0f);

    glm::mat4 viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();
    glm::mat4 invViewProj = glm::inverse(viewProj);

    glm::vec4 worldCoords = invViewProj * clipCoords;

    return glm::vec2(worldCoords.x, worldCoords.y);
}
