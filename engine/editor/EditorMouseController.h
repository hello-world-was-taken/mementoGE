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

    // handlers
    void handleLeftClickSelection(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos);
    void handleRightClickPopup(EditorContext &ctx, Scene &scene, glm::vec2 mouseWorldPos);
    void handleDragging(EditorContext &ctx, Scene &scene, glm::vec2 dragStartPos, glm::vec2 mouseWorldPos);
    void handleZoom(EditorContext &ctx);

    void selectObjectsInDrag(EditorContext &ctx, Scene &scene, glm::vec2 worldDragStart, glm::vec2 mouseWorldPos);

private:
    // based on m_mode, move the game object to snap to grid or free
    void moveSelectedGameObjects(EditorContext &ctx, glm::vec2 mouseWorldPos);
    void moveCamera(EditorContext &ctx);

    MovementMode m_movementMode = MovementMode::Free;
    glm::vec2 m_dragOffset;
};
