#include "core/components/Transform.h"

#include "core/GLIncludes.h"
#include "core/Scene.h"
#include "core/SceneManager.h"

#include "editor/EditorCamera.h"
#include "editor/EditorContext.h"
#include "editor/EditorInteractionMode.h"
#include "editor/EditorMouseController.h"

#include "util/Time.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <iostream>

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
    EditorContext &ctx, ImVec2 imagePos, ImVec2 imageSize, int framebufferWidth, int framebufferHeight)
{
    if (!ctx.sceneImageHovered)
        return;

    Scene &scene = ctx.sceneManager.getActiveScene();
    EditorCamera &editorCamera = ctx.editorCamera;

    auto &gameObjects = scene.getGameObjects();

    MouseListener *mouse = MouseListener::instance();
    glm::vec2 mouseWorldPos = getWorldCoordinate(
        editorCamera, imagePos, imageSize, framebufferWidth, framebufferHeight, mouse->getMouseScreenPosition());
    glm::vec2 dragStartPos = getWorldCoordinate(
        editorCamera, imagePos, imageSize, framebufferWidth, framebufferHeight, mouse->getDragStart());

    handleLeftClickSelection(ctx, scene, mouseWorldPos);
    handleRightClickPopup(ctx, scene, mouseWorldPos);
    handleDragging(ctx, scene, dragStartPos, mouseWorldPos, framebufferWidth, framebufferHeight);
    handleZoom(editorCamera, framebufferWidth, framebufferHeight);
}

void EditorMouseController::handleLeftClickSelection(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos)
{
    MouseListener *mouse = MouseListener::instance();
    if (!mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
        return;

    ctx.selectedObjects.clear();

    auto clickedObject = getGameObjectAt(scene, mouseWorldPos);
    if (clickedObject)
    {
        ctx.selectedObjects.push_back(clickedObject->get());
        glm::vec3 objPos = clickedObject->get().getComponent<Transform>().position;
        m_dragOffset = glm::vec2(objPos.x, objPos.y) - mouseWorldPos;
    }
}

void EditorMouseController::handleRightClickPopup(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos)
{
    MouseListener *mouse = MouseListener::instance();
    if (!mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
        return;

    ctx.selectedObjects.clear();
    ctx.showPropertiesPopup = false;

    ImVec2 mousePos = ImGui::GetMousePos();
    auto clickedObject = getGameObjectAt(scene, mouseWorldPos);

    if (clickedObject)
    {
        ctx.selectedObjects.push_back(clickedObject->get());
        ctx.showPropertiesPopup = true;

        ctx.propertiesPopupPos = ImVec2(mousePos.x + 15, mousePos.y);
        ImGui::OpenPopup("PropertiesPopup");
    }
    else
    {
        ctx.showCreateObjectPopup = true;
        ctx.createObjectPopupPos = ImVec2(mousePos.x, mousePos.y);
        ctx.createObjectWorldPos = mouseWorldPos;
        ImGui::OpenPopup("CreateObjectPopup");
    }
}

void EditorMouseController::handleZoom(EditorCamera &camera, int framebufferWidth, int framebufferHeight)
{
    MouseListener *mouse = MouseListener::instance();

    // Adjust zoom level
    camera.adjustZoom(mouse->getScrollDelta().x);
    camera.onViewportResize(framebufferWidth, framebufferHeight);
}

void EditorMouseController::handleDragging(EditorContext &ctx,
    Scene &scene,
    glm::vec2 dragStartPos,
    glm::vec2 mouseWorldPos,
    int framebufferWidth,
    int framebufferHeight)
{
    MouseListener *mouse = MouseListener::instance();

    if (mouse->isMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT))
    {
        switch (ctx.interactionMode)
        {
        case EditorInteractionMode::Selection:
            selectObjectsInDrag(ctx, scene, dragStartPos, mouseWorldPos);
            break;

        case EditorInteractionMode::Gliding:
            moveCamera(ctx, framebufferWidth, framebufferHeight);
            break;

        case EditorInteractionMode::MoveObjects:
            moveSelectedGameObjects(ctx, mouseWorldPos);
            break;

        default:
            break;
        }
    }
}

void EditorMouseController::moveSelectedGameObjects(EditorContext &ctx, glm::vec2 mouseWorldPos)
{
    for (const auto &go : ctx.selectedObjects)
    {
        if (m_movementMode == MovementMode::SnapToGrid)
        {
            // Assume grid size is equal to the object's width.
            float gridSize = static_cast<float>(go.get().getWidth());

            // This should be kept in sync with GridRenderer.cpp
            float snappedX = std::floor(mouseWorldPos.x / gridSize) * gridSize;
            float snappedY = std::floor(mouseWorldPos.y / gridSize) * gridSize;

            Transform &transform = go.get().getComponent<Transform>();
            transform.position = {snappedX, snappedY, transform.position.z};
        }
        else if (m_movementMode == MovementMode::Free)
        {
            glm::vec2 newPos = mouseWorldPos + m_dragOffset;

            Transform &transform = go.get().getComponent<Transform>();
            transform.position = {newPos.x, newPos.y, transform.position.z};
        }
    }
}

void EditorMouseController::moveCamera(EditorContext &ctx, int framebufferWidth, int framebufferHeight)
{
    EditorCamera &editorCamera = ctx.editorCamera;

    // Scale screen (window) coordinates to framebuffer space
    int winWidth, winHeight;
    glfwGetWindowSize(ctx.window.getGlfwWindow(), &winWidth, &winHeight);

    float scaleX = static_cast<float>(framebufferWidth) / winWidth;
    float scaleY = static_cast<float>(framebufferHeight) / winHeight;

    glm::vec2 dragDelta = MouseListener::instance()->getMouseDelta();

    editorCamera.setPosition(editorCamera.getPosition() - glm::vec3(dragDelta.x * scaleX, dragDelta.y * scaleY, 0.0f));
}

std::optional<std::reference_wrapper<GameObject>> EditorMouseController::getGameObjectAt(
    Scene &scene, glm::vec2 mouseWorldPos)
{
    for (const auto &obj : scene.getGameObjects())
    {
        GameObject *topObject = nullptr;
        float topZ = -std::numeric_limits<float>::infinity();

        for (auto &obj : scene.getGameObjects())
        {
            if (obj.containsPoint(mouseWorldPos))
            {
                Transform &transform = obj.getComponent<Transform>();
                float z = transform.position.z;

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

glm::vec2 EditorMouseController::getWorldCoordinate(const Camera &camera,
    ImVec2 imagePos,
    ImVec2 imageSize,
    int framebufferWidth,
    int framebufferHeight,
    glm::vec2 mouseScreenCoords)
{
    glm::vec2 localPos = screenToLocal(mouseScreenCoords, {imagePos.x, imagePos.y}, {imageSize.x, imageSize.y});
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

glm::vec2 EditorMouseController::localToFrameBuffer(
    glm::vec2 localPos, glm::vec2 imageSize, int framebufferWidth, int framebufferHeight)
{
    float fbX = (localPos.x / imageSize.x) * framebufferWidth;
    float fbY = (localPos.y / imageSize.y) * framebufferHeight;

    return {fbX, fbY};
}

glm::vec2 EditorMouseController::frameBufferToWorld(
    const Camera &camera, glm::vec2 fbPos, int framebufferWidth, int framebufferHeight)
{
    // Convert to Normalized Device Coordinates (NDC)
    float ndcX = (fbPos.x / framebufferWidth) * 2.0f - 1.0f;
    float ndcY = (fbPos.y / framebufferHeight) * 2.0f - 1.0f;

    // TODO: do we need to update z to layer images over one another?
    glm::vec4 clipCoords = glm::vec4(ndcX, ndcY, 0.0f, 1.0f);

    glm::mat4 viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();
    glm::mat4 invViewProj = glm::inverse(viewProj);

    glm::vec4 worldCoords = invViewProj * clipCoords;

    return glm::vec2(worldCoords.x, worldCoords.y);
}

void EditorMouseController::selectObjectsInDrag(
    EditorContext &ctx, Scene &scene, glm::vec2 worldDragStart, glm::vec2 mouseWorldPos)
{
    ctx.selectedObjects.clear();

    glm::vec2 minPoint = glm::min(worldDragStart, mouseWorldPos);
    glm::vec2 maxPoint = glm::max(worldDragStart, mouseWorldPos);

    for (auto &obj : scene.getGameObjects())
    {
        Transform &transform = obj.getComponent<Transform>();
        glm::vec2 pos = {transform.position.x, transform.position.y};
        glm::vec2 size = {obj.getWidth(), obj.getHeight()};

        glm::vec2 objMin = pos - size * 0.5f;
        glm::vec2 objMax = pos + size * 0.5f;

        bool intersects =
            (objMin.x <= maxPoint.x && objMax.x >= minPoint.x) && (objMin.y <= maxPoint.y && objMax.y >= minPoint.y);

        if (intersects)
        {
            ctx.selectedObjects.push_back(obj);
        }
    }
}