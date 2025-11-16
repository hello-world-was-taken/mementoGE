#include "core/components/EntityInfo.h"
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

void EditorMouseController::update(EditorContext &ctx)
{
    if (!ctx.sceneImageHovered)
    {
        return;
    }

    Scene &scene = ctx.sceneManager.getActiveScene();
    EditorCamera &editorCamera = ctx.editorCamera;

    auto &gameObjects = scene.getGameObjects();

    MouseListener *mouse = MouseListener::instance();
    glm::vec2 mouseWorldPos = ctx.getWorldCoordinate(mouse->getMouseScreenPosition());
    glm::vec2 dragStartPos = ctx.getWorldCoordinate(mouse->getDragStart());

    handleLeftClickSelection(ctx, scene, mouseWorldPos);
    handleRightClickPopup(ctx, scene, mouseWorldPos);
    handleDragging(ctx, scene, dragStartPos, mouseWorldPos);
    handleZoom(ctx);
}

void EditorMouseController::handleLeftClickSelection(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos)
{
    MouseListener *mouse = MouseListener::instance();
    if (!mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        return;
    }

    bool inSelectionMode = ctx.interactionMode == EditorInteractionMode::Selection;
    if (inSelectionMode)
    {
        return;
    }

    // On the first left button click store the objects' offset
    bool storeObjectOffsets = ctx.interactionMode == EditorInteractionMode::MoveObjects;
    if (storeObjectOffsets)
    {
        ctx.selectedGameObjectsDragOffset.clear();

        for (GameObject &obj : ctx.selectedObjects)
        {
            glm::vec3 objPos = obj.getComponent<Transform>().position;
            glm::vec2 dragOffset = glm::vec2(objPos.x, objPos.y) - mouseWorldPos;
            ctx.selectedGameObjectsDragOffset.push_back(dragOffset);
        }

        return;
    }

    ctx.selectedObjects.clear();
    ctx.selectedGameObjectsDragOffset.clear();

    auto clickedObject = getGameObjectAt(scene, mouseWorldPos);
    if (clickedObject)
    {
        glm::vec3 objPos = clickedObject->get().getComponent<Transform>().position;
        glm::vec2 dragOffset = glm::vec2(objPos.x, objPos.y) - mouseWorldPos;
        ctx.selectedGameObjectsDragOffset.push_back(dragOffset);
        ctx.selectedObjects.push_back(clickedObject->get());
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

void EditorMouseController::handleZoom(EditorContext &ctx)
{
    MouseListener *mouse = MouseListener::instance();

    // Adjust zoom level
    ctx.editorCamera.adjustZoom(mouse->getScrollDelta().x);
    ctx.editorCamera.onViewportResize(ctx.frameBuffer.getWidth(), ctx.frameBuffer.getHeight());
}

void EditorMouseController::handleDragging(
    EditorContext &ctx, Scene &scene, glm::vec2 dragStartPos, glm::vec2 mouseWorldPos)
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
            moveCamera(ctx);
            break;

        // NOTE: fall-through
        case EditorInteractionMode::MoveObjects:
        default:
            ctx.performSceneEdit(
                [&]
                {
                    moveSelectedGameObjects(ctx, mouseWorldPos);
                });
            break;
        }
    }
}

void EditorMouseController::moveSelectedGameObjects(EditorContext &ctx, glm::vec2 mouseWorldPos)
{
    for (int i = 0; i < ctx.selectedObjects.size(); ++i)
    {
        const auto &go = ctx.selectedObjects[i];
        glm::vec2 dragOffset = ctx.selectedGameObjectsDragOffset[i];

        if (m_movementMode == MovementMode::SnapToGrid)
        {
            // TODO: for now this would work as a way to snap objects for alignment purposes
            // but we can also use it for grid based tiles
            float gridSize = static_cast<float>(5);

            // This should be kept in sync with GridRenderer.cpp
            float snappedX = std::floor(mouseWorldPos.x / gridSize) * gridSize;
            float snappedY = std::floor(mouseWorldPos.y / gridSize) * gridSize;

            Transform &transform = go.get().getComponent<Transform>();
            transform.position = {snappedX, snappedY, transform.position.z};
        }
        else if (m_movementMode == MovementMode::Free)
        {
            glm::vec2 newPos = mouseWorldPos + dragOffset;

            Transform &transform = go.get().getComponent<Transform>();
            transform.position = {newPos.x, newPos.y, transform.position.z};
        }
    }
}

void EditorMouseController::moveCamera(EditorContext &ctx)
{
    EditorCamera &editorCamera = ctx.editorCamera;

    // Scale screen (window) coordinates to framebuffer space
    int winWidth, winHeight;
    glfwGetWindowSize(ctx.window.getGlfwWindow(), &winWidth, &winHeight);

    float scaleX = ctx.frameBuffer.getWidth() / winWidth;
    float scaleY = ctx.frameBuffer.getHeight() / winHeight;

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

void EditorMouseController::selectObjectsInDrag(
    EditorContext &ctx, Scene &scene, glm::vec2 worldDragStart, glm::vec2 mouseWorldPos)
{
    ctx.selectedObjects.clear();

    glm::vec2 minPoint = glm::min(worldDragStart, mouseWorldPos);
    glm::vec2 maxPoint = glm::max(worldDragStart, mouseWorldPos);

    for (auto &obj : scene.getGameObjects())
    {
        EntityInfo &entityInfo = obj.getComponent<EntityInfo>();
        Transform &transform = obj.getComponent<Transform>();
        glm::vec2 pos = {transform.position.x, transform.position.y};
        glm::vec2 size = {entityInfo.width, entityInfo.height};

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