#include "SceneManager.h"


SceneManager::SceneManager(GLFWwindow *window)
{
    this->window = window;
    this->scenes = new std::map<const char *, Scene *>();
    this->addScene("triangle_scene", new Scene());
}


SceneManager::~SceneManager()
{
    for (auto &scene : *scenes)
    {
        delete scene.second;
    }
}


void SceneManager::loadScene(const char *sceneName)
{
    std::cout << "Loading scene: " << sceneName << std::endl;
    activeScene = scenes->at(sceneName);
    std::cout << "Scene Loaded" << sceneName << std::endl;
    activeScene->setBufferData();
    activeScene->draw(window);
}


void SceneManager::unloadScene(const char *sceneName)
{
    activeScene = nullptr;
}


void SceneManager::addScene(const char *sceneName, Scene *scene)
{
    std::cout << "Adding scene: " << sceneName << std::endl;
    (*scenes)[sceneName] = scene;
    std::cout << "Scene Added" << sceneName << std::endl;
}