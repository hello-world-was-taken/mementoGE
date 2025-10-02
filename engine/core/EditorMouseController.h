#pragma once
#include "core/MouseListener.h"
#include "core/GameObject.h"
#include "core/Scene.h"
#include "core/MovementMode.h"

#include <memory>
#include <imgui.h>

class EditorContext;

class EditorMouseController
{
public:
    EditorMouseController();

    void setMovementMode(MovementMode mode);
    void SetActiveObject(GameObject &object);

    // camera required for world-space conversion
    // game object required for selection detection
    void Update(EditorContext &ctx, ImVec2 imagePos, ImVec2 imageSize, int framebufferWidth, int framebufferHeight);

    glm::vec2 getWorldCoordinate(std::shared_ptr<Camera> camera, ImVec2 imagePos, ImVec2 imageSize, int framebufferWidth, int framebufferHeight);

    /*
     * screen here is glfw window
     * local here is scene preview imgui window
    */
    glm::vec2 screenToLocal(glm::vec2 mousePos, glm::vec2 imagePos, glm::vec2 imageSize);
    glm::vec2 localToFrameBuffer(glm::vec2 localPos, glm::vec2 imageSize, int framebufferWidth, int framebufferHeight);
    glm::vec2 frameBufferToWorld(std::shared_ptr<Camera> camera, glm::vec2 fbPos, int framebufferWidth, int framebufferHeight);

private:
    // based on m_mode, move the game object to snap to grid or free
    void moveGameObject(GameObject *activeGameObject, glm::vec2 mouseWorldPos);
    void moveCamera(EditorContext &ctx, int framebufferWidth, int framebufferHeight);

    MovementMode m_movementMode = MovementMode::Free;
    glm::vec2 m_dragOffset;
};
