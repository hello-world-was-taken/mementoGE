#pragma once

#include "core/GLIncludes.h"
#include "core/Scene.h"
#include "core/Window.h"

#include <map>
#include <memory>
#include <optional>

class SceneManager
{
public:
    SceneManager(Window *window);
    ~SceneManager();

    void start();
    void update();

    void loadScene(std::string sceneName);
    // TODO: what should we do when we unload a scene?
    void unloadScene(std::string sceneName);
    void addScene(std::string sceneName, Scene &&scene);
    void removeScene(std::string sceneName);

    void getScene(std::string sceneName);
    Scene &getActiveScene();
    void setActiveScene(std::string sceneName);

    void startRuntimeScene();
    void pauseRuntimeScene();
    void stopRuntimeScene();
    bool isPlaying() const;

    void serialize();
    void deserialize();

    // TODO: make this private. for debugging
    bool m_isPlaying = false;

private:

    std::map<std::string, Scene> m_scenes;
    std::optional<std::string> m_activeSceneName;
    Window *m_window = nullptr;
};