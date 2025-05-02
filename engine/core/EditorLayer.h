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
    EditorLayer(Window &window, const EventHandler &eventHandler);
    ~EditorLayer();

    void onAttach();                // Called when the editor initializes
    void onUpdate(float deltaTime); // Called every frame
    void onImGuiRender();           // Render ImGui windows here

    void setScene(std::shared_ptr<Scene> scene);

private:
    void renderSceneViewport();
    void renderPropertiesPanel();
    void renderSelectedTexSheetPanel();
    void renderTextureListPanel();
    void renderGrid();
    void renderPerformancePanel();

    void handleSceneInteraction();
    void handleEvents();

    std::vector<std::string> getTextureFiles(const std::string &folderPath);

    // Viewport settings
    int m_viewportWidth = 1280;
    int m_viewportHeight = 720;

    // logical game world screen size. Should be kept in sync with scene.h
    float m_screen_width = 32.0f * 16.0f; // 16 tiles of 32 pixels = 512 pixels
    float m_screen_height = 32.0f * 9.0f; // 9 tiles of 32 pixels = 288 pixels

    std::map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::shared_ptr<Scene> m_currentScene;
    std::string m_selectedTexturePath;
    std::shared_ptr<Camera> m_editorCamera;

    MouseListener *m_mouseListener = nullptr;
    MouseActionController m_mouseActionController;
    const EventHandler &m_eventHandler;
    GridRenderer m_gridRenderer{static_cast<int>(m_screen_width), static_cast<int>(m_screen_height), 32};

    Window &m_window;

    FrameBuffer m_frameBuffer{m_viewportWidth, m_viewportHeight};
};
