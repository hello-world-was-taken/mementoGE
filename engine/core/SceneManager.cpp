#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>

#include "core/SceneManager.h"
#include "core/Resource.h"
#include "core/Sprite.h"
#include "core/SpriteSheet.h"
#include "core/GameObject.h"
#include "core/Camera.h"

SceneManager::SceneManager(Window *window)
    : m_window{window}
{
}

SceneManager::~SceneManager()
{
    std::cout << "SceneManager destructor called" << std::endl;
    serialize();
}

void SceneManager::start()
{
    // TODO: improve the it to avoid such a gymnastics
    std::shared_ptr<Camera> camera = getActiveScene().getCamera();
    m_window->setUserData(camera.get());

    getActiveScene().start();
}

void SceneManager::update()
{
    getActiveScene().update(Time::deltaTime(), m_window->getGlfwWindow());
}

void SceneManager::loadScene(std::string sceneName)
{
    auto it = m_scenes.find(sceneName);
    if (it != m_scenes.end())
    {
        m_activeSceneName = sceneName;
        getActiveScene().start();
    }
    else
    {
        std::cout << "Scene not found: " << sceneName << std::endl;
    }
}

void SceneManager::unloadScene(std::string sceneName)
{
    m_activeSceneName = std::nullopt;
}

void SceneManager::addScene(std::string sceneName, Scene &&scene)
{
    m_scenes.insert({sceneName, std::move(scene)});
    // TODO: shouldn't the active scene be set to the latest scene added?
    if (!m_activeSceneName.has_value())
    {
        m_activeSceneName = sceneName;
    }
}

void SceneManager::removeScene(std::string sceneName)
{
    m_scenes.erase(sceneName);
}

Scene &SceneManager::getActiveScene()
{
    if (m_activeSceneName.has_value())
    {
        return m_scenes.find(m_activeSceneName.value())->second;
    }
    throw std::runtime_error("No active scene found.");
}

void SceneManager::serialize()
{
    if (m_scenes.size() == 0)
    {
        return;
    }

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
    // TODO: active scene should be handled better.
    m_activeSceneName = "default_scene";

    std::cout << "Deserialized scene from scene.yaml" << std::endl;
}