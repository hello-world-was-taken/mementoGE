#pragma once

#include "core/EventHandler.h"
#include "core/Scene.h"
#include "core/Window.h"

#include "editor/EditorContext.h"
#include "editor/panels/AssetsPanel.h"
#include "editor/panels/GameObjectEditorPanel.h"
#include "editor/panels/LogPanel.h"
#include "editor/panels/PropertiesPanel.h"
#include "editor/panels/SceneHierarchyPanel.h"
#include "editor/panels/ScenePanel.h"
#include "editor/panels/SpriteSheetEditorPanel.h"
#include "editor/panels/SystemsPanel.h"
#include "editor/panels/TileMapEditorPanel.h"

#include "renderer/GridRenderer.h"
#include "renderer/PhysicsRenderer.h"
#include "renderer/SelectionRenderer.h"
#include "renderer/SpriteRenderer.h"

#include "opengl/FrameBuffer.h"

#include <memory>

class EditorLayer
{
public:
    EditorLayer();
    ~EditorLayer();

    void run();
    void drawEditorUI(); // Render ImGui windows here

    void setScene(std::shared_ptr<Scene> scene);
    EditorContext &getEditorContext();

private:
    void updateFrame(const FrameContext &frame);
    void renderMainMenuBar();

    // imgui panels
    void renderGrid();
    void renderPerformancePanel();
    void renderEditorProperties();
    CameraOld &getPlayCamera(Scene &scene);

    void drawMouseDebugPanel();

    std::vector<std::string> getTextureFiles(const std::string &folderPath);

    void handleEditorShortcuts(const InputSnapshot &input);

private:
    Window m_window;
    EditorContext m_ctx{m_window};

    ScenePanel m_scenePanel{m_ctx};
    SceneHierarchyPanel m_sceneHierarchyPanel{m_ctx};
    PropertiesPanel m_propertiesPanel{m_ctx};
    AssetsPanel m_assetsPanel{m_ctx};
    LogPanel m_logPanel{m_ctx};
    SystemsPanel m_systemsPanel{m_ctx};
    SpriteSheetEditorPanel m_spriteSheetEditorPanel{m_ctx};
    TileMapEditorPanel m_tileMapEditorPanel{m_ctx};
    GameObjectEditorPanel m_gameObjectEditorPanel{m_ctx};

    GridRenderer m_gridRenderer;
    CameraOld m_playCamera;

    glm::vec4 editorModeClearColor = {0.41176f, 0.41176f, 0.41176f, 1.00f};
    glm::vec4 playModeClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
};
