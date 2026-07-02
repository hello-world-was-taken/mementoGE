#include "core/components/EntityInfo.h"
#include "core/components/Transform.h"

#include "core/Scene.h"
#include "core/SceneManager.h"

#include "editor/EditorCamera.h"
#include "editor/EditorContext.h"
#include "editor/EditorInteractionMode.h"
#include "editor/EditorMouseController.h"

#include "util/LogChannels.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <sawyer/Logger.h>

EditorMouseController::EditorMouseController() = default;

void EditorMouseController::update(EditorContext &ctx)
{
    Scene &scene = ctx.getActiveScene();
    for (const Event &event : ctx.frame.input.events)
    {
        // A release must finish an existing drag even after the cursor leaves the scene image.
        if (event.type == EventType::MouseButtonRelease && event.mouseButton == MouseButton::Left)
        {
            finishDragging(ctx);
            continue;
        }
        if (!ctx.sceneImageHovered || !event.insideSceneViewport)
        {
            continue;
        }

        const glm::vec2 mouseScreenPos{event.mouseX, event.mouseY};
        if (event.type == EventType::MouseButtonPress)
        {
            const glm::vec2 mouseWorldPos = ctx.getWorldCoordinate(mouseScreenPos);
            if (event.mouseButton == MouseButton::Left)
            {
                m_leftDragActive = true;
                m_dragStart = mouseScreenPos;
                handleSingleLeftClick(ctx, scene, mouseWorldPos);
            }
            else if (event.mouseButton == MouseButton::Right)
            {
                handleRightClick(ctx, scene, mouseWorldPos, {event.mouseX, event.mouseY});
            }
        }
        else if (event.type == EventType::MouseScroll)
        {
            handleZoom(ctx, event.scrollY);
        }
        else if (m_leftDragActive &&
                 (event.type == EventType::MouseMove ||
                     (event.type == EventType::MouseButtonHeld && event.mouseButton == MouseButton::Left)))
        {
            const glm::vec2 delta = event.type == EventType::MouseMove
                                       ? glm::vec2{event.mouseDeltaX, event.mouseDeltaY}
                                       : glm::vec2{0.0f};
            handleDragging(ctx,
                scene,
                ctx.getWorldCoordinate(m_dragStart),
                ctx.getWorldCoordinate(mouseScreenPos),
                delta);
        }
    }
}

void EditorMouseController::handleSingleLeftClick(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos)
{
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

void EditorMouseController::handleRightClick(
    EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos, ImVec2 mouseScreenPos)
{
    resetRightClickState(ctx);
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
}

void EditorMouseController::handleRightClickOnEmpty(
    EditorContext &ctx, glm::vec2 mouseWorldPos, const ImVec2 &mouseScreenPos)
{
    ctx.showCreateObjectPopup = true;

    ctx.createObjectPopupPos = mouseScreenPos;
    ctx.createObjectWorldPos = mouseWorldPos;
}

void EditorMouseController::handleZoom(EditorContext &ctx, float scrollDelta)
{
    if (scrollDelta != 0.0f)
    {
        ctx.editorCamera.adjustZoom(scrollDelta);
    }
}

void EditorMouseController::handleDragging(
    EditorContext &ctx, Scene &scene, glm::vec2 dragStartPos, glm::vec2 mouseWorldPos, glm::vec2 mouseDelta)
{
    switch (ctx.interactionMode)
    {
    case EditorInteractionMode::Selection:
        selectObjectsInDrag(ctx, scene, dragStartPos, mouseWorldPos);
        break;

    case EditorInteractionMode::Gliding:
        moveCamera(ctx, mouseDelta);
        break;

    case EditorInteractionMode::MoveObjects:
    case EditorInteractionMode::None:
        moveSelectedGameObjects(ctx, mouseWorldPos);
        m_isDraggingObjects = true;
        break;
    default:
        break;
    }
}

void EditorMouseController::finishDragging(EditorContext &ctx)
{
    if (m_isDraggingObjects)
    {
        ctx.snapshotScene();
    }
    m_isDraggingObjects = false;
    m_leftDragActive = false;
}

void EditorMouseController::moveSelectedGameObjects(EditorContext &ctx, glm::vec2 mouseWorldPos)
{
    if (ctx.selectedObjects.size() != ctx.selectedGameObjectsDragOffset.size())
    {
        LOG_WARN(LogChannel::Editor,
            "moveSelectedGameObjects invalid state: {} selected objects, {} drag offsets",
            ctx.selectedObjects.size(),
            ctx.selectedGameObjectsDragOffset.size());
        return;
    }

    for (int i = 0; i < ctx.selectedObjects.size(); ++i)
    {
        GameObject &go = ctx.selectedObjects[i];
        glm::vec2 dragOffset = ctx.selectedGameObjectsDragOffset[i];

        if (ctx.movementMode == MovementMode::SnapToGrid)
        {
            // TODO: for now this would work as a way to snap objects for alignment purposes
            // but we can also use it for grid based tiles
            float gridSize = static_cast<float>(16);
            float halfGrid = gridSize * 0.5f;

            // Using center-based convention to match what we have for gameobjects GameObject::getQuad()
            float snappedX = std::floor(mouseWorldPos.x / gridSize) * gridSize + halfGrid;
            float snappedY = std::floor(mouseWorldPos.y / gridSize) * gridSize + halfGrid;

            Transform &transform = go.getComponent<Transform>();
            transform.position = {snappedX, snappedY, transform.position.z};
        }
        else if (ctx.movementMode == MovementMode::Free)
        {
            glm::vec2 newPos = mouseWorldPos + dragOffset;

            Transform &transform = go.getComponent<Transform>();
            transform.position = {newPos.x, newPos.y, transform.position.z};
        }
    }
}

void EditorMouseController::moveCamera(EditorContext &ctx, glm::vec2 mouseDelta)
{
    // Scale captured window-coordinate motion into framebuffer space.
    const auto [winWidth, winHeight] = ctx.frame.window.getWindowSize();
    if (winWidth <= 0 || winHeight <= 0)
    {
        return;
    }
    const float scaleX = static_cast<float>(ctx.renderer2D.m_finalFBO.getWidth()) / winWidth;
    const float scaleY = static_cast<float>(ctx.renderer2D.m_finalFBO.getHeight()) / winHeight;
    ctx.editorCamera.setPosition(
        ctx.editorCamera.getPosition() - glm::vec3(mouseDelta.x * scaleX, mouseDelta.y * scaleY, 0.0f));
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
