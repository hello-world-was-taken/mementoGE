#pragma once

#include "core/Camera.h"
#include "core/EventHandler.h"
#include "core/MovementMode.h"
#include "core/Scene.h"
#include "core/SceneManager.h"
#include "core/Window.h"

#include "editor/EditorContext.h"
#include "editor/PropertiesPanel.h"
#include "editor/SceneHierarchyPanel.h"
#include "editor/ScenePanel.h"
#include "editor/TexturePanel.h"

#include "renderer/GridRenderer.h"
#include "renderer/PhysicsRenderer.h"
#include "renderer/SelectionRenderer.h"
#include "renderer/SpriteRenderer.h"

#include "opengl/FrameBuffer.h"

#include <memory>

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
    void renderAddNewObjectPopup();

    void drawMouseDebugPanel();

    std::vector<std::string> getTextureFiles(const std::string &folderPath);

    void undoRedoListener();

private:
    EditorContext m_ctx;
    ScenePanel m_scenePanel;
    SceneHierarchyPanel m_sceneHierarchyPanel;
    TexturePanel m_texturePanel;
    PropertiesPanel m_propertiesPanel;

    MovementMode m_movementMode = MovementMode::Free;
    bool m_drawGrid = false;

    GridRenderer m_gridRenderer;
    PhysicsRenderer m_physicsRenderer;
    SelectionRenderer m_selectionRenderer;
    SpriteRenderer m_spriteRenderer;
};
