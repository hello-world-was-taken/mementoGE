#pragma once

#include <memory>

#include "core/Scene.h"
#include "core/SceneManager.h"
#include "core/Camera.h"
#include "core/MouseActionController.h"
#include "core/Window.h"
#include "core/EventHandler.h"
#include "core/MovementMode.h"
#include "core/EditorContext.h"
#include "core/ScenePanel.h"

#include "renderer/GridRenderer.h"
#include "renderer/PhysicsRenderer.h"

#include "opengl/FrameBuffer.h"

class EditorLayer
{
public:
    EditorLayer(Window &window);
    ~EditorLayer();

    void onAttach();                // Called when the editor initializes
    void onUpdate(float deltaTime); // Called every frame
    void drawEditorUI();            // Render ImGui windows here

    void setScene(std::shared_ptr<Scene> scene);
    // TODO: a copy of the scene manager. Keep in sync for now.
    void serialize();

private:
    // imgui panels
    void renderSceneViewport();
    void renderPropertiesPanel();
    void renderSelectedTexSheetPanel();
    void renderSelectedTexSheetPanel(bool isInModal);
    void renderTextureListPanel();
    void renderChooseFile();
    void renderGrid();
    void renderPerformancePanel();
    // void renderGizmos();
    void renderEditorProperties();

    void handleSceneInteraction();
    void handleEvents();

    std::vector<std::string> getTextureFiles(const std::string &folderPath);

private:
    EditorContext m_ctx;
    ScenePanel m_scenePanel;

    MovementMode m_movementMode = MovementMode::Free;
    bool m_drawGrid = false;

    // logical game world screen size. Should be kept in sync with scene.h
    float m_screen_width = 32.0f * 16.0f; // 16 tiles of 32 pixels = 512 pixels
    float m_screen_height = 32.0f * 9.0f; // 9 tiles of 32 pixels = 288 pixels

    GridRenderer m_gridRenderer;
    PhysicsRenderer m_physicsRenderer{};
};
