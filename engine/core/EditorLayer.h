#pragma once

#include <memory>

#include "core/Scene.h"
#include "core/Camera.h"
#include "core/MouseListener.h"
#include "core/MouseActionController.h"
#include "core/Window.h"
#include "core/EventHandler.h"
#include "renderer/GridRenderer.h"

#include "opengl/FrameBuffer.h"

class EditorLayer
{
public:
    EditorLayer(Window &window);
    ~EditorLayer();

    void onAttach();                // Called when the editor initializes
    void onUpdate(float deltaTime); // Called every frame
    void onImGuiRender();           // Render ImGui windows here

    void setScene(std::shared_ptr<Scene> scene);
    //TODO: a copy of the scene manager. Keep in sync for now.
    void serialize();

private:
    void renderSceneViewport();
    void renderPropertiesPanel();
    void renderSelectedTexSheetPanel();
    void renderTextureListPanel();
    void renderChooseFile();
    void renderGrid();
    void renderPerformancePanel();
    void renderGizmos();

    void handleSceneInteraction();
    void handleEvents();

    std::vector<std::string> getTextureFiles(const std::string &folderPath);

    // get the screen coordinates (glfw screen) from world coordinate
    // since gizmos are imgui rendered
    glm::vec2 getScreenCoordinate(glm::vec2 worldPos);
    glm::vec2 worldToFrameBuffer(glm::vec2 worldPos);
    glm::vec2 frameBufferToLocal(glm::vec2 frameBufferPos);
    // screen -> glfw window, local -> scene preview imgui window
    glm::vec2 localToScreen(glm::vec2 localPos);

    // Viewport settings
    int m_viewportWidth = 1280;
    int m_viewportHeight = 720;

    // logical game world screen size. Should be kept in sync with scene.h
    float m_screen_width = 32.0f * 16.0f; // 16 tiles of 32 pixels = 512 pixels
    float m_screen_height = 32.0f * 9.0f; // 9 tiles of 32 pixels = 288 pixels

    // scene preview points
    ImVec2 m_upperLeft;
    ImVec2 m_previewAreaSize;

    std::map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::shared_ptr<Scene> m_currentScene;
    std::string m_selectedTexturePath;
    std::shared_ptr<Camera> m_editorCamera;

    MouseActionController m_mouseActionController;
    GridRenderer m_gridRenderer;

    Window &m_window;

    FrameBuffer m_frameBuffer{m_viewportWidth, m_viewportHeight};
};
