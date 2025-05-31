#include "core/Scene.h"
#include "core/MouseListener.h" // TODO: Game specific things shouldn't be here.

#include "renderer/RenderBatch.h"

#include <iostream>
#include <string>
#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>
#include <stdexcept>

Scene::Scene(std::string &&tag)
    : mTag{tag}, m_isPaused{false}
{
}

Scene::Scene(const YAML::Node &&serializedScene)
    : m_registry{}
{
    // Deserializing
    for (YAML::const_iterator it = serializedScene.begin(); it != serializedScene.end(); ++it)
    {
        mTag = it->first.as<std::string>();

        YAML::Node gameObjects = it->second["Game Objects"];

        for (YAML::const_iterator it = gameObjects.begin(); it != gameObjects.end(); ++it)
        {
            GameObject gameObj{m_registry, it->second, m_physicsWorld};
            m_gameObjects.push_back(std::move(gameObj));
        }

        // TODO: FIX THIS!
        m_activeEntityId = m_gameObjects.back().getEntityId();
    }
}

Scene::Scene(Scene &&other)
    : m_registry{std::move(other.m_registry)}
{
    m_screen_height = other.m_screen_height;
    m_screen_width = other.m_screen_width;
    m_camera = std::move(other.m_camera);
    m_textures = std::move(other.m_textures);
    mTag = std::move(other.mTag);

    for (GameObject &g : other.m_gameObjects)
    {
        g.updateEntityReference(m_registry);
        m_gameObjects.push_back(std::move(g));
    }

    // setting other pointer variables to null_ptr
    other.m_activeEntityId = entt::null;
    other.m_registry = NULL;

    // TODO: DO THIS IN A BETTER WAY
    if (m_gameObjects.size())
        m_activeEntityId = m_gameObjects.back().getEntityId();
}

Scene &Scene::operator=(Scene &&other)
{
    m_screen_height = other.m_screen_height;
    m_screen_width = other.m_screen_width;
    m_registry = std::move(other.m_registry);
    m_gameObjects = std::move(other.m_gameObjects);
    m_camera = std::move(other.m_camera);
    m_textures = std::move(other.m_textures);
    m_activeEntityId = other.m_activeEntityId;
    mTag = std::move(other.mTag);

    // setting other pointer variables to null_ptr
    other.m_activeEntityId = entt::null;

    return *this;
}

Scene::~Scene()
{
    std::cout << "Scene destructor called: " << mTag << std::endl;
}

Scene Scene::clone(std::string tag)
{
    YAML::Emitter out;
    std::string sceneTag = mTag;
    mTag = tag;

    serialize(out);
    mTag = sceneTag;

    YAML::Node serializedScene = YAML::Load(out.c_str());

    return Scene{std::move(serializedScene)};
}

void Scene::start()
{
}

void Scene::update(float deltaTime, GLFWwindow *window)
{
    if (!m_isPaused)
    {
        m_physicsWorld.simulate(deltaTime, m_gameObjects);
        m_physicsWorld.syncTransforms(m_gameObjects);
    }

    m_spriteRenderer.setActiveGameObjects(&getGameObjects());
    m_spriteRenderer.setCamera(m_camera);
    m_spriteRenderer.render();
}

void Scene::setPaused(bool paused)
{
    m_isPaused = paused;
}

bool Scene::isPaused() const
{
    return m_isPaused;
}

void Scene::addGameObject(unsigned int width, unsigned int height, std::string &&tag)
{
    auto go = GameObject{m_registry, std::move(tag), width, height};
    m_activeEntityId = go.getEntityId();
    m_gameObjects.push_back(std::move(go));
}

std::vector<GameObject> &Scene::getGameObjects()
{
    return m_gameObjects;
}

std::shared_ptr<Camera> Scene::getCamera() const
{
    return m_camera;
}

GameObject *Scene::getActiveGameObject()
{
    for (auto &go : m_gameObjects)
    {
        if (go.getEntityId() == m_activeEntityId)
        {
            return &go;
        }
    }

    return nullptr;
}

void Scene::setActiveGameObject(entt::entity entityId)
{
    m_activeEntityId = entityId;
}

// TODO: this adds rigid bodies again and again. Fix it
void Scene::addRigidBody2DToWorld()
{
    GameObject *go = getActiveGameObject();
    m_physicsWorld.addRigidbody(*go);
}

const std::string &Scene::getTag() const
{
    return mTag;
}

bool Scene::serialize(YAML::Emitter &out)
{
    out << YAML::BeginMap;
    out << YAML::Key << mTag;
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "Game Objects";
    out << YAML::Value << YAML::BeginMap;

    for (GameObject &gameObject : m_gameObjects)
    {
        gameObject.serialize(out);
    }

    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::EndMap;

    // TODO: Do we need this to return bool?
    return true;
}