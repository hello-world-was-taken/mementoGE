#pragma once

#include "core/EventHandler.h"
#include "core/Scene.h"
#include "core/Window.h"

#include "editor/AssetsPanel.h"
#include "editor/EditorContext.h"
#include "editor/PropertiesPanel.h"
#include "editor/SceneHierarchyPanel.h"
#include "editor/ScenePanel.h"
#include "editor/SpriteSheetEditorPanel.h"

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
    void updateFrame();

    // imgui panels
    void renderGrid();
    void renderPerformancePanel();
    void renderEditorProperties();
    void renderAddNewObjectPopup();

    void drawMouseDebugPanel();

    std::vector<std::string> getTextureFiles(const std::string &folderPath);

    void handleEditorShortcuts();

private:
    Window m_window;
    EditorContext m_ctx{m_window};

    ScenePanel m_scenePanel{m_ctx};
    SceneHierarchyPanel m_sceneHierarchyPanel{m_ctx};
    PropertiesPanel m_propertiesPanel{m_ctx};
    AssetsPanel m_assetsPanel{m_ctx};
    SpriteSheetEditorPanel m_spriteSheetEditorPanel{m_ctx};

    bool m_drawGrid = false;

    GridRenderer m_gridRenderer;

    glm::vec4 editorModeClearColor = {0.41176f, 0.41176f, 0.41176f, 1.00f};
    glm::vec4 playModeClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
};
