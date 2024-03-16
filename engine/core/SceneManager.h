#pragma once

#include <map>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "engine/core/Scene.h"
#include "engine/core/Window.h"

class SceneManager;

// NOTE: A function type that returs void and takes glfw window and scene manager as argument.
using EventHandlerFunction = void (*)(GLFWwindow *, SceneManager *);

class SceneManager
{
private:
    std::map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::shared_ptr<Scene> m_activeScene;
    Window *m_window;
    EventHandlerFunction m_eventHandlerFunction;

public:
    SceneManager(Window *window);
    ~SceneManager();

    void start();
    void update(float deltaTime);
    void gameLoop();
    void setEventHandler(EventHandlerFunction eventHandler);
    void loadScene(const char *sceneName);
    void unloadScene(const char *sceneName);
    void addScene(const char *sceneName, std::shared_ptr<Scene> scene);
    void removeScene(const char *sceneName);
    void renderTextureResourcesImGui();
    void getScene(const char *sceneName);
    std::shared_ptr<Scene> getActiveScene();
    void setActiveScene(const char *sceneName);
    void serialize();
    void deserialize();
};