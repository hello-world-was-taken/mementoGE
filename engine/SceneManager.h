#pragma once

#include <map>
#include <memory>
#include "Scene.h"
#include "Window.h"

class SceneManager
{
private:
    std::map<const char *, std::shared_ptr<Scene>> scenes;
    std::shared_ptr<Scene> activeScene;
    GLFWwindow *window;

public:
    SceneManager(GLFWwindow *window);
    ~SceneManager();

    void start();
    void update(float deltaTime, GLFWwindow *window);
    void loadScene(const char *sceneName);
    void unloadScene(const char *sceneName);
    void addScene(const char *sceneName, std::shared_ptr<Scene> scene);
    void removeScene(const char *sceneName);
    void getScene(const char *sceneName);
    std::shared_ptr<Scene> getActiveScene();
    void setActiveScene(const char *sceneName);
    void getSceneCount();
    void getSceneNames();
};