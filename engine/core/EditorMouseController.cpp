#include "core/GLIncludes.h"
#include "core/Scene.h"
#include "core/Transform.h"
#include "core/SceneManager.h"
#include "core/EditorMouseController.h"
#include "core/EditorContext.h"

#include "util/Time.h"

#include <iostream>
#include <cmath>
#include <imgui.h>

EditorMouseController::EditorMouseController()
{
}

void EditorMouseController::setMovementMode(MovementMode mode)
{
    m_movementMode = mode;
}

void EditorMouseController::SetActiveObject(GameObject &object)
{
}

void EditorMouseController::Update(
    EditorContext &ctx,
    ImVec2 imagePos,
    ImVec2 imageSize,
    int framebufferWidth,
    int framebufferHeight)
{
    if (!ctx.sceneImageHovered)
        return;

    Scene &scene = ctx.sceneManager.getActiveScene();
    std::shared_ptr<Camera> camera = scene.getCamera();

    auto &gameObjects = scene.getGameObjects();
    auto prevActiveGameObject = scene.getActiveGameObject();

    MouseListener *mouse = MouseListener::instance();
    glm::vec2 mouseWorldPos = getWorldCoordinate(camera, imagePos, imageSize, framebufferWidth, framebufferHeight);

    // check for button click on an object
    if (mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        auto clickedObject = getGameObjectAt(scene, mouseWorldPos);

        if (clickedObject)
        {
            scene.setActiveGameObject(clickedObject->get().getEntityId());
            auto activeGameObject = scene.getActiveGameObject();

            // store offset between object origin and mouse world position to avoid initial sharp mov't
            if (activeGameObject)
            {
                glm::vec3 *objPos = activeGameObject->getComponent<Transform>().getPosition();
                m_dragOffset = glm::vec2(objPos->x, objPos->y) - mouseWorldPos;
            }
        }
        else
        {
            // the user clicked on empty space
            scene.setActiveGameObject(entt::null);
        }
    }

    // handle right-click for to bringup properties editor
    if (mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        ctx.showPropertiesPopup = false;
        auto clickedObject = getGameObjectAt(scene, mouseWorldPos);

        if (clickedObject)
        {
            scene.setActiveGameObject(clickedObject->get().getEntityId());
            ctx.showPropertiesPopup = true;

            // store popup position
            ImVec2 mousePos = ImGui::GetMousePos();
            ctx.propertiesPopupPos = ImVec2(mousePos.x + 15, mousePos.y);

            ImGui::OpenPopup("PropertiesPopup");
        }
    }

    // handle object being dragged
    auto currActiveGameObject = scene.getActiveGameObject();
    bool similarGameObjectClicked = (prevActiveGameObject && currActiveGameObject) && (prevActiveGameObject->getEntityId() == currActiveGameObject->getEntityId());

    bool draggingOnGameObject = mouse->isMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT) && similarGameObjectClicked;
    if (draggingOnGameObject)
    {
        moveGameObject(currActiveGameObject, mouseWorldPos);
    }

    bool draggingOnEmptySpace = mouse->isMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT) && !currActiveGameObject;
    if (draggingOnEmptySpace)
    {
        moveCamera(ctx, framebufferWidth, framebufferHeight);
    }

    bool leftButtonClicked = mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || mouse->isMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT);

    // handle zooming in and out of the editor
    // TODO: improve the camera API
    camera->adjustZoom(mouse->getScrollDelta().x);
    camera->updateProjection(framebufferWidth, framebufferHeight);
}

void EditorMouseController::moveGameObject(GameObject *activeGameObject, glm::vec2 mouseWorldPos)
{
    if (m_movementMode == MovementMode::SnapToGrid)
    {
        // Assume grid size is equal to the object's width.
        float gridSize = static_cast<float>(activeGameObject->getWidth());

        // This should be kept in sync with GridRenderer.cpp
        float snappedX = std::floor(mouseWorldPos.x / gridSize) * gridSize;
        float snappedY = std::floor(mouseWorldPos.y / gridSize) * gridSize;

        Transform &transform = activeGameObject->getComponent<Transform>();
        transform.setPosition(snappedX, snappedY, transform.getPosition()->z);
    }
    else if (m_movementMode == MovementMode::Free)
    {
        glm::vec2 newPos = mouseWorldPos + m_dragOffset;

        Transform &transform = activeGameObject->getComponent<Transform>();
        transform.setPosition(newPos.x, newPos.y, transform.getPosition()->z);
    }
}

void EditorMouseController::moveCamera(EditorContext &ctx, int framebufferWidth, int framebufferHeight)
{
    Scene &scene = ctx.sceneManager.getActiveScene();
    std::shared_ptr<Camera> camera = scene.getCamera();

    // Scale screen (window) coordinates to framebuffer space
    int winWidth, winHeight;
    glfwGetWindowSize(ctx.window.getGlfwWindow(), &winWidth, &winHeight);

    float scaleX = static_cast<float>(framebufferWidth) / winWidth;
    float scaleY = static_cast<float>(framebufferHeight) / winHeight;

    glm::vec2 dragDelta = MouseListener::instance()->getMouseDelta();

    camera->setPosition(camera->getPosition() - glm::vec3(dragDelta.x * scaleX, dragDelta.y * scaleY, 0.0f));
}

std::optional<std::reference_wrapper<const GameObject>>
EditorMouseController::getGameObjectAt(Scene &scene, glm::vec2 mouseWorldPos)
{
    for (const auto &obj : scene.getGameObjects())
    {
        const GameObject *topObject = nullptr;
        float topZ = -std::numeric_limits<float>::infinity();

        for (const auto &obj : scene.getGameObjects())
        {
            if (obj.containsPoint(mouseWorldPos))
            {
                Transform &transform = obj.getComponent<Transform>();
                float z = transform.getPosition()->z;

                if (z > topZ) // pick the object with the highest z
                {
                    topZ = z;
                    topObject = &obj;
                }
            }
        }

        if (topObject)
            return *topObject; // reference_wrapper auto-constructed
    }
    return std::nullopt;
}

glm::vec2 EditorMouseController::getWorldCoordinate(std::shared_ptr<Camera> camera, ImVec2 imagePos, ImVec2 imageSize, int framebufferWidth, int framebufferHeight)
{
    MouseListener *listener = MouseListener::instance();

    // Mouse position in screen coordinates
    glm::vec2 mousePos = listener->getMouseScreenPosition();
    glm::vec2 localPos = screenToLocal(mousePos, {imagePos.x, imagePos.y}, {imageSize.x, imageSize.y});
    glm::vec2 fbPos = localToFrameBuffer(localPos, {imageSize.x, imageSize.y}, framebufferWidth, framebufferHeight);

    // Make sure the mouse is inside the image
    if (localPos.x < 0 || localPos.y < 0 || localPos.x > imageSize.x || localPos.y > imageSize.y)
        return glm::vec2(-1.0f);

    return frameBufferToWorld(camera, fbPos, framebufferWidth, framebufferHeight);
}

glm::vec2 EditorMouseController::screenToLocal(glm::vec2 mousePos, glm::vec2 imagePos, glm::vec2 imageSize)
{
    // Get position of mouse relative to imgui scene preview window
    float localX = mousePos.x - imagePos.x;
    float localY = mousePos.y - imagePos.y;

    // Flip Y because ImGui has origin at top-left, OpenGL at bottom-left
    localY = imageSize.y - localY;

    return {localX, localY};
}

glm::vec2 EditorMouseController::localToFrameBuffer(glm::vec2 localPos, glm::vec2 imageSize, int framebufferWidth, int framebufferHeight)
{
    float fbX = (localPos.x / imageSize.x) * framebufferWidth;
    float fbY = (localPos.y / imageSize.y) * framebufferHeight;

    return {fbX, fbY};
}

glm::vec2 EditorMouseController::frameBufferToWorld(std::shared_ptr<Camera> camera, glm::vec2 fbPos, int framebufferWidth, int framebufferHeight)
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
