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
    activeScene = scenes->at(sceneName);
    activeScene->setBufferData();
    activeScene->draw(window);
}


void SceneManager::unloadScene(const char *sceneName)
{
    activeScene = nullptr;
}


void SceneManager::addScene(const char *sceneName, Scene *scene)
{
    (*scenes)[sceneName] = scene;
}