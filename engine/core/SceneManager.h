#pragma once

#include <map>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "core/Scene.h"
#include "core/Window.h"

class SceneManager;

// NOTE: A function type that returs void and takes glfw window and scene manager as argument.
using EventHandlerFunction = void (*)(GLFWwindow *, SceneManager *);

class SceneManager
{
public:
    SceneManager(Window *window);
    ~SceneManager();

    void start();
    void update(float deltaTime);
    void gameLoop();
    void setEventHandler(EventHandlerFunction eventHandler);
    void loadScene(const char *sceneName);
    void unloadScene(const char *sceneName);
    void addScene(const char *sceneName, Scene &scene);
    void removeScene(const char *sceneName);
    void renderTextureResourcesImGui();
    void getScene(const char *sceneName);
    Scene *getActiveScene();
    void setActiveScene(const char *sceneName);
    void serialize();
    void deserialize();

private:
    std::map<std::string, Scene> m_scenes;
    Scene *m_activeScene = nullptr;
    Window *m_window = nullptr;
    EventHandlerFunction m_eventHandlerFunction;
};