#include "SceneManager.h"


SceneManager::SceneManager(GLFWwindow *window)
{
    this->window = window;
    this->scenes = new std::map<const char *, Scene *>();
    this->addScene("triangle_scene", new Scene());
    activeScene = scenes->at("triangle_scene");
}


SceneManager::~SceneManager()
{
    for (auto &scene : *scenes)
    {
        delete scene.second;
    }
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
    activeScene = scenes->at(sceneName);
    activeScene->start(window);
}


void SceneManager::unloadScene(const char *sceneName)
{
    activeScene = nullptr;
}


void SceneManager::addScene(const char *sceneName, Scene *scene)
{
    (*scenes)[sceneName] = scene;
}

void SceneManager::removeScene(const char *sceneName)
{
    scenes->erase(sceneName);
}

Scene* SceneManager::getActiveScene()
{
    return activeScene;
}