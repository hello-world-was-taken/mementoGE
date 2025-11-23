#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

#include "opengl/FrameBuffer.h"

#include <memory>
#include <imgui.h>

class ScenePanel : public EditorPanel
{
public:
    ScenePanel(EditorContext &ctx);
    ~ScenePanel();

    void draw() override;

private:
    EditorContext &m_ctx;

private:
    void renderGizmos();
    void renderPlayPause();
    void renderMovementMode();
    void renderSceneViewport();
    void handleViewportDropTarget();
    void createSpriteFromPayload(const ImGuiPayload *payload);
};