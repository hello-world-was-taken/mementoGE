#include "engine/core/SceneManager.h"

SceneManager::SceneManager(GLFWwindow *window)
{
    this->window = window;
    // this->addScene("triangle_scene", std::make_shared<Scene>());
    // activeScene = scenes.at("triangle_scene");
}

SceneManager::~SceneManager()
{
    std::cout << "SceneManager destructor called" << std::endl;
}

void SceneManager::start()
{
    if (activeScene == nullptr)
    {
        std::cout << "No active scene found" << std::endl;
        return;
    }
    activeScene->start(this->window);
}

void SceneManager::update(float deltaTime, GLFWwindow *window)
{
    activeScene->update(deltaTime, window);
}

void SceneManager::loadScene(const char *sceneName)
{
    auto it = scenes.find(sceneName);
    if (it != scenes.end())
    {
        activeScene = it->second;
        activeScene->start(window);
    }
    else
    {
        std::cout << "Scene not found: " << sceneName << std::endl;
    }
}

void SceneManager::unloadScene(const char *sceneName)
{
    activeScene = nullptr;
}

void SceneManager::addScene(const char *sceneName, std::shared_ptr<Scene> scene)
{
    scenes[sceneName] = scene;
    if (activeScene == nullptr)
    {
        activeScene = scenes[sceneName];
    }
}

void SceneManager::removeScene(const char *sceneName)
{
    scenes.erase(sceneName);
}

std::shared_ptr<Scene> SceneManager::getActiveScene()
{
    return activeScene;
}