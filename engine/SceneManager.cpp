#include "SceneManager.h"

SceneManager::SceneManager(GLFWwindow *window)
{
    this->window = window;
    this->addScene("triangle_scene", std::make_shared<Scene>());
    activeScene = scenes.at("triangle_scene");
}

SceneManager::~SceneManager()
{
    std::cout << "SceneManager destructor called" << std::endl;
}

void SceneManager::start()
{
    loadScene("triangle_scene");
    activeScene->start(this->window);
    this->getActiveScene()->addGameObject();
}

void SceneManager::update(float deltaTime, GLFWwindow *window)
{
    activeScene->update(deltaTime, window);
}

void SceneManager::loadScene(const char *sceneName)
{
    activeScene = scenes.at(sceneName);
    activeScene->start(window);
}

void SceneManager::unloadScene(const char *sceneName)
{
    activeScene = nullptr;
}

void SceneManager::addScene(const char *sceneName, std::shared_ptr<Scene> scene)
{
    scenes[sceneName] = scene;
}

void SceneManager::removeScene(const char *sceneName)
{
    scenes.erase(sceneName);
}

std::shared_ptr<Scene> SceneManager::getActiveScene()
{
    return activeScene;
}