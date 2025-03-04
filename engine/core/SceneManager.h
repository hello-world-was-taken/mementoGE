#pragma once

#include <map>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "core/Scene.h"
#include "core/Window.h"
#include "core/EventHandler.h"

class SceneManager;

// NOTE: A function type that returs void and takes glfw window and scene manager as argument.
using EventHandlerFunction = void (*)(Window &, SceneManager *, const EventHandler &);

class SceneManager
{
public:
    SceneManager(Window *window, const EventHandler &eventHandler);
    ~SceneManager();

    void start();
    void update(float deltaTime);
    void gameLoop();

    /**
     * Add a new game object to active scene
     */
    bool addGameObject(int x, int y);

    /**
     * Add game object components to active scene's
     * active game object
     */
    template <typename Component, typename... Args>
    bool addComponent(Args &&...args)
    {
        m_activeScene->getActiveGameObject().addComponent<Component>(std::forward<Args>(args)...);
    }

    GameObject& getActiveGameObject() const;
    void setEventHandler(EventHandlerFunction eventHandler);
    void loadScene(const char *sceneName);
    // TODO: what should we do when we unload a scene?
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
    const EventHandler &mEventHandler;
    EventHandlerFunction m_eventHandlerFunction;
};