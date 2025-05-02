#pragma once

#include <memory>

#include "core/Scene.h"
#include "core/Camera.h"
#include "core/MouseListener.h"
#include "core/MouseActionController.h"
#include "core/Window.h"
#include "core/EventHandler.h"

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
    void renderResourcesPanel();
    void handleSceneInteraction();
    void handleEvents();

    // Viewport settings
    int m_viewportWidth = 1280;
    int m_viewportHeight = 720;

    std::map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::shared_ptr<Scene> m_currentScene;
    std::shared_ptr<Camera> m_editorCamera;

    MouseListener *m_mouseListener = nullptr;
    MouseActionController m_mouseActionController;
    const EventHandler &m_eventHandler;

    Window &m_window;

    FrameBuffer m_frameBuffer{m_viewportWidth, m_viewportHeight};
};
