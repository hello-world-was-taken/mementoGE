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

    Scene &scene = ctx.getActiveScene();
    EditorCamera &editorCamera = ctx.editorCamera;

    MouseListener *mouse = MouseListener::instance();
    glm::vec2 mouseWorldPos = ctx.getWorldCoordinate(mouse->getMouseScreenPosition());
    glm::vec2 dragStartPos = ctx.getWorldCoordinate(mouse->getDragStart());

    handleSingleLeftClick(ctx, scene, mouseWorldPos);
    handleRightClick(ctx, scene, mouseWorldPos);
    handleDragging(ctx, scene, dragStartPos, mouseWorldPos);
    handleZoom(ctx);
}

void EditorMouseController::handleSingleLeftClick(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos)
{
    MouseListener *mouse = MouseListener::instance();

    if (!mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        return;
    }

    if (isInSelectionMode(ctx))
    {
        return;
    }

    if (isMovementMode(ctx))
    {
        storeDragOffsets(ctx, mouseWorldPos);
        return;
    }

    clearSelection(ctx);
    selectClickedObject(ctx, scene, mouseWorldPos);
}

bool EditorMouseController::isInSelectionMode(const EditorContext &ctx) const
{
    return ctx.interactionMode == EditorInteractionMode::Selection;
}

bool EditorMouseController::isMovementMode(const EditorContext &ctx) const
{
    return ctx.interactionMode == EditorInteractionMode::MoveObjects;
}

// On the first left button click store the objects' offset
void EditorMouseController::storeDragOffsets(EditorContext &ctx, glm::vec2 mouseWorldPos)
{
    ctx.selectedGameObjectsDragOffset.clear();

    for (GameObject &obj : ctx.selectedObjects)
    {
        glm::vec3 objPos = obj.getComponent<Transform>().position;
        glm::vec2 offset = glm::vec2(objPos.x, objPos.y) - mouseWorldPos;
        ctx.selectedGameObjectsDragOffset.push_back(offset);
    }
}

void EditorMouseController::clearSelection(EditorContext &ctx)
{
    ctx.selectedObjects.clear();
    ctx.selectedGameObjectsDragOffset.clear();
}

void EditorMouseController::selectClickedObject(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos)
{
    auto clickedObject = getGameObjectAt(scene, mouseWorldPos);
    if (!clickedObject)
    {
        return;
    }

    GameObject &obj = clickedObject->get();
    glm::vec3 objPos = obj.getComponent<Transform>().position;

    glm::vec2 dragOffset = glm::vec2(objPos.x, objPos.y) - mouseWorldPos;

    ctx.selectedGameObjectsDragOffset.push_back(dragOffset);
    ctx.selectedObjects.push_back(obj);
}

void EditorMouseController::handleRightClick(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos)
{
    MouseListener *mouse = MouseListener::instance();
    if (!mouse->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        return;
    }

    resetRightClickState(ctx);

    ImVec2 mouseScreenPos = ImGui::GetMousePos();
    auto clickedObject = getGameObjectAt(scene, mouseWorldPos);

    if (clickedObject)
    {
        handleRightClickOnObject(ctx, clickedObject->get(), mouseScreenPos);
    }
    else
    {
        handleRightClickOnEmpty(ctx, mouseWorldPos, mouseScreenPos);
    }
}

void EditorMouseController::resetRightClickState(EditorContext &ctx)
{
    ctx.selectedObjects.clear();
    ctx.selectedGameObjectsDragOffset.clear();
    ctx.showPropertiesPopup = false;
}

void EditorMouseController::handleRightClickOnObject(EditorContext &ctx, GameObject &obj, const ImVec2 &mouseScreenPos)
{
    ctx.selectedObjects.push_back(obj);
    ctx.showPropertiesPopup = true;

    ctx.propertiesPopupPos = ImVec2(mouseScreenPos.x + 15, mouseScreenPos.y);
    ImGui::OpenPopup("PropertiesPopup");
}

void EditorMouseController::handleRightClickOnEmpty(
    EditorContext &ctx, glm::vec2 mouseWorldPos, const ImVec2 &mouseScreenPos)
{
    ctx.showCreateObjectPopup = true;

    ctx.createObjectPopupPos = mouseScreenPos;
    ctx.createObjectWorldPos = mouseWorldPos;

    ImGui::OpenPopup("CreateObjectPopup");
}

void EditorMouseController::handleZoom(EditorContext &ctx)
{
    MouseListener *mouse = MouseListener::instance();

    // Adjust zoom level
    ctx.editorCamera.adjustZoom(mouse->getScrollDelta().x);
    ctx.editorCamera.onViewportResize(ctx.renderer2D.m_finalFBO.getWidth(), ctx.renderer2D.m_finalFBO.getHeight());
}

void EditorMouseController::handleDragging(
    EditorContext &ctx, Scene &scene, glm::vec2 dragStartPos, glm::vec2 mouseWorldPos)
{
    MouseListener *mouse = MouseListener::instance();

    bool leftHeld = mouse->isMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT);
    bool leftReleased = mouse->wasMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT);

    if (leftHeld)
    {
        switch (ctx.interactionMode)
        {
        case EditorInteractionMode::Selection:
            selectObjectsInDrag(ctx, scene, dragStartPos, mouseWorldPos);
            break;

        case EditorInteractionMode::Gliding:
            moveCamera(ctx);
            break;

        // Note fall-through
        case EditorInteractionMode::MoveObjects:
        case EditorInteractionMode::None:
            moveSelectedGameObjects(ctx, mouseWorldPos);
            m_isDraggingObjects = true;
            break;
        default:
            break;
        }
    }

    // Take a single snapshot when an object-move drag finishes
    if (leftReleased)
    {
        if ((ctx.interactionMode == EditorInteractionMode::MoveObjects ||
                ctx.interactionMode == EditorInteractionMode::None) &&
            m_isDraggingObjects)
        {
            ctx.snapshotScene();
        }

        m_isDraggingObjects = false;
    }
}

void EditorMouseController::moveSelectedGameObjects(EditorContext &ctx, glm::vec2 mouseWorldPos)
{
    std::cout << "Moving object" << std::endl;
    if (ctx.selectedObjects.size() != ctx.selectedGameObjectsDragOffset.size())
    {
        std::cout << "moveSelectedGameObjects invalid state" << std::endl;
        std::cout << ctx.selectedObjects.size() << " : " << ctx.selectedGameObjectsDragOffset.size() << std::endl;
        return;
    }

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

    float scaleX = ctx.renderer2D.m_finalFBO.getWidth() / winWidth;
    float scaleY = ctx.renderer2D.m_finalFBO.getHeight() / winHeight;

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
        {
            return *topObject; // reference_wrapper auto-constructed
        }
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