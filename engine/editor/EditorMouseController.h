#pragma once

#include "core/Camera.h"
#include "core/GameObject.h"
#include "core/MouseListener.h"
#include "core/MovementMode.h"
#include "core/Scene.h"

#include <functional>
#include <imgui.h>
#include <memory>

class EditorContext;

class EditorMouseController
{
public:
    EditorMouseController();

    void setMovementMode(MovementMode mode);

    void update(EditorContext &ctx);
    std::optional<std::reference_wrapper<GameObject>> getGameObjectAt(Scene &scene, glm::vec2 mouseWorldPos);

    // only handles the single click. If user drags, it won't be handled here
    void handleSingleLeftClick(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos);
    void handleRightClick(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos);
    void handleDragging(EditorContext &ctx, Scene &scene, glm::vec2 dragStartPos, glm::vec2 mouseWorldPos);
    void handleZoom(EditorContext &ctx);

    void selectObjectsInDrag(EditorContext &ctx, Scene &scene, glm::vec2 worldDragStart, glm::vec2 mouseWorldPos);

private:
    bool isInSelectionMode(const EditorContext &ctx) const;
    bool isMovementMode(const EditorContext &ctx) const;
    void storeDragOffsets(EditorContext &ctx, glm::vec2 mouseWorldPos);
    void clearSelection(EditorContext &ctx);
    void selectClickedObject(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos);

    void resetRightClickState(EditorContext &ctx);
    void handleRightClickOnObject(EditorContext &ctx, GameObject &obj, const ImVec2 &mouseScreenPos);
    void handleRightClickOnEmpty(EditorContext &ctx, glm::vec2 mouseWorldPos, const ImVec2 &mouseScreenPos);
    // based on m_mode, move the game object to snap to grid or free
    void moveSelectedGameObjects(EditorContext &ctx, glm::vec2 mouseWorldPos);
    void moveCamera(EditorContext &ctx);

    MovementMode m_movementMode = MovementMode::Free;
    bool m_isDraggingObjects = false;
};
