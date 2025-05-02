#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>

#include "core/SceneManager.h"
#include "core/Resource.h"
#include "core/Sprite.h"
#include "core/SpriteSheet.h"
#include "core/GameObject.h"
#include "core/Camera.h"

SceneManager::SceneManager(Window *window, const EventHandler &eventHandler)
    : m_window{window},
      mEventHandler{eventHandler}
{
}

SceneManager::~SceneManager()
{
    std::cout << "SceneManager destructor called" << std::endl;
}

void SceneManager::start()
{
    if (m_activeScene == nullptr)
    {
        std::cout << "No active m_scene found" << std::endl;
        return;
    }
    // TODO: improve the it to avoid such a gymnastics
    std::shared_ptr<Camera> camera = m_activeScene->getCamera();
    m_window->setUserData(camera.get());

    m_activeScene->start();
}

void SceneManager::update()
{
    m_activeScene->update(Time::deltaTime(), m_window->getGlfwWindow());
}

void SceneManager::loadScene(const char *sceneName)
{
    auto it = m_scenes.find(sceneName);
    if (it != m_scenes.end())
    {
        m_activeScene = &(it->second);
        m_activeScene->start();
    }
    else
    {
        std::cout << "Scene not found: " << sceneName << std::endl;
    }
}

void SceneManager::unloadScene(const char *sceneName)
{
    m_activeScene = nullptr;
}

void SceneManager::addScene(const char *sceneName, Scene &&scene)
{
    m_scenes.insert({sceneName, std::move(scene)});
    // TODO: shouldn't the active scene be set to the latest scene added?
    if (m_activeScene == nullptr)
    {
        m_activeScene = &(m_scenes.find(sceneName)->second);
    }
}

void SceneManager::removeScene(const char *sceneName)
{
    m_scenes.erase(sceneName);
}

Scene *SceneManager::getActiveScene()
{
    return m_activeScene;
}

void SceneManager::serialize()
{
    YAML::Emitter out;
    out << YAML::BeginMap;

    // iterate through the map of scenes
    for (auto &[sceneName, scene] : m_scenes)
    {
        out << YAML::Key << sceneName;
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Game Objects";
        out << YAML::Value << YAML::BeginMap;

        for (GameObject &gameObject : scene.getGameObjects())
        {
            gameObject.serialize(out);
        }

        out << YAML::EndMap;
    }

    out << YAML::EndMap;
    std::ofstream file("../game/scene.yaml", std::ios::out | std::ios::trunc);
    file << out.c_str();

    std::cout << "Serialized scene to scene.yaml" << std::endl;
}

void SceneManager::deserialize()
{
    // TODO: if we update our serialization method and scene.yaml doesn't adhere to that
    //       we should throw an error or remove the file
    // if a file called ../game/scene.yaml does not exist, return
    if (!std::filesystem::exists("../game/scene.yaml"))
    {
        std::cout << "scene.yaml does not exist" << std::endl;
        return;
    }

    // TODO: We are assuming we'll always have a single scene. fix.
    YAML::Node serializedScene = YAML::LoadFile("../game/scene.yaml");
    Scene scene = Scene{std::move(serializedScene)};

    m_scenes.insert_or_assign("default_scene", std::move(scene));
    // TODO: map.find() returns iterator end if the value is not present. Fix it.
    // TODO: active scene should be handled better.
    m_activeScene = &(m_scenes.find("default_scene")->second);

    std::cout << "Deserialized scene from scene.yaml" << std::endl;
}