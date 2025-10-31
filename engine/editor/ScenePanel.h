#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

#include "opengl/FrameBuffer.h"

#include <memory>

class ScenePanel : public EditorPanel
{
public:
    ScenePanel(EditorContext &ctx);
    ~ScenePanel();

    void draw() override;

private:
    EditorContext &m_ctx;

    // scene preview points
    ImVec2 m_upperLeft;
    ImVec2 m_previewAreaSize;

private:
    void renderGizmos();
    void renderPlayPause();
    void renderMovementMode();
    void renderSceneViewport();

    // get the screen coordinates (glfw screen) from world coordinate
    // since gizmos are imgui rendered
    glm::vec2 getScreenCoordinate(glm::vec2 worldPos);
    glm::vec2 worldToFrameBuffer(glm::vec2 worldPos);
    glm::vec2 frameBufferToLocal(glm::vec2 frameBufferPos);
    // screen -> glfw window, local -> scene preview imgui window
    glm::vec2 localToScreen(glm::vec2 localPos);
};