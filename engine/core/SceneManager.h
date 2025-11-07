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

    void update();

    void loadScene(std::string sceneName);
    void unloadScene(std::string sceneName);
    void addScene(std::string sceneName, Scene &&scene);
    void removeScene(std::string sceneName);

    void getScene(std::string sceneName);
    void setActiveScene(std::string sceneName);
    Scene &getActiveScene();

    void startRuntimeScene();
    void pauseRuntimeScene();
    void stopRuntimeScene();
    bool isPlaying() const;

    void serialize();
    void deserialize();

    // TODO: make this private. for debugging
    bool m_isPlaying = false;

    std::map<std::string, Scene> m_scenes;
private:
    std::optional<std::string> m_activeSceneName;
    Window *m_window = nullptr;
};