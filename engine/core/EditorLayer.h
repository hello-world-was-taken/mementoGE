#pragma once

#include <memory>

#include "core/Scene.h"
#include "core/SceneManager.h"
#include "core/Camera.h"
#include "core/Window.h"
#include "core/EventHandler.h"
#include "core/MovementMode.h"
#include "core/EditorContext.h"
#include "core/ScenePanel.h"
#include "core/PropertiesPanel.h"
#include "core/TexturePanel.h"

#include "renderer/GridRenderer.h"
#include "renderer/PhysicsRenderer.h"
#include "renderer/SpriteRenderer.h"

#include "opengl/FrameBuffer.h"

class EditorLayer
{
public:
    EditorLayer(Window &window);
    ~EditorLayer();

    void update();
    void drawEditorUI(); // Render ImGui windows here

    void setScene(std::shared_ptr<Scene> scene);
    EditorContext &getEditorContext();

private:
    // imgui panels
    void renderGrid();
    void renderPerformancePanel();
    void renderEditorProperties();
    void drawMouseDebugPanel();

    void handleSceneInteraction();
    void handleEvents();

    std::vector<std::string> getTextureFiles(const std::string &folderPath);

private:
    EditorContext m_ctx;
    ScenePanel m_scenePanel;
    TexturePanel m_texturePanel;
    PropertiesPanel m_propertiesPanel;

    MovementMode m_movementMode = MovementMode::Free;
    bool m_drawGrid = false;

    GridRenderer m_gridRenderer;
    PhysicsRenderer m_physicsRenderer;
    SpriteRenderer m_spriteRenderer;
};
