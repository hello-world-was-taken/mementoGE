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
        auto it = m_scenes.find(m_activeSceneName.value());
        if (it != m_scenes.end())
        {
            return it->second;
        }
    }
    throw std::runtime_error("No active scene found.");
}

void SceneManager::startRuntimeScene()
{
    auto it = m_scenes.find("runtime_scene");
    if (it == m_scenes.end())
    {
        m_scenes.insert({"runtime_scene", getActiveScene().clone("runtime_scene")});
    }
    m_activeSceneName = "runtime_scene";
    getActiveScene().play();
    std::cout
        << "startRuntimeScene tag: " << getActiveScene().getTag() << std::endl;
    m_isPlaying = true;
}

void SceneManager::pauseRuntimeScene()
{
    getActiveScene().pause();
    m_isPlaying = false;
}

void SceneManager::stopRuntimeScene()
{
    m_scenes.erase("runtime_scene");
    m_activeSceneName = "default_scene";
    m_isPlaying = false;
}

bool SceneManager::isPlaying() const
{
    return m_isPlaying;
}

void SceneManager::serialize()
{
    if (m_scenes.size() == 0)
    {
        return;
    }

    YAML::Emitter out;

    for (auto &[sceneName, scene] : m_scenes)
    {
        scene.serialize(out);
    }

    std::ofstream file("../game/scene.yaml", std::ios::out | std::ios::trunc);
    file << out.c_str();

    std::cout << "Serialized scene to scene.yaml" << std::endl;
}

void SceneManager::deserialize()
{
    const std::string sceneFile = "../game/scene.yaml";

    if (!std::filesystem::exists(sceneFile))
    {
        std::cout << "scene.yaml does not exist" << std::endl;
        Scene scene{"default_scene"};
        m_scenes.insert_or_assign(scene.getTag(), std::move(scene));
        m_activeSceneName = scene.getTag();
        return;
    }

    YAML::Node serializedScene = YAML::LoadFile(sceneFile);

    // Check if loaded node is valid and not empty
    if (!serializedScene.IsDefined() || serializedScene.size() == 0)
    {
        std::cerr << "scene.yaml is empty or invalid" << std::endl;
        Scene scene{"default_scene"};
        m_activeSceneName = scene.getTag();
        m_scenes.insert_or_assign(scene.getTag(), std::move(scene));
        return;
    }

    Scene scene{std::move(serializedScene)};
    m_activeSceneName = scene.getTag();
    m_scenes.insert_or_assign(scene.getTag(), std::move(scene));
    std::cout << "Deserialized scene from scene.yaml" << std::endl;
}