#pragma once

#include <map>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "core/Scene.h"
#include "core/Window.h"
#include "core/MouseActionController.h"
#include "opengl/FrameBuffer.h"

class SceneManager
{
public:
    SceneManager(Window *window);
    ~SceneManager();

    void start();
    void update();

    void loadScene(const char *sceneName);
    // TODO: what should we do when we unload a scene?
    void unloadScene(const char *sceneName);
    void addScene(const char *sceneName, Scene &&scene);
    void removeScene(const char *sceneName);

    void getScene(const char *sceneName);
    Scene *getActiveScene();
    void setActiveScene(const char *sceneName);

    void serialize();
    void deserialize();

private:
    std::map<std::string, Scene> m_scenes;
    Scene *m_activeScene = nullptr;
    Window *m_window = nullptr;
    MouseActionController mMouseActionController;
};