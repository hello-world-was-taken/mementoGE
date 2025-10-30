#include "core/components/Sprite.h"

#include "core/Scene.h"
#include "core/Animator.h"

#include "renderer/RenderBatch.h"

#include "util/Time.h"

#include <iostream>
#include <string>
#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>
#include <stdexcept>

Scene::Scene(std::string &&tag)
    : mTag{tag}, m_play{false}
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

        for (const auto &it : gameObjects)
        {
            GameObject gameObj{m_registry, it.second, m_physicsWorld};
            m_gameObjects.push_back(std::move(gameObj));
        }

        // TODO: FIX THIS! Exception is thrown when scene.yaml doesn't have any objects
        m_activeEntityId = m_gameObjects.back().getEntityId();
    }
}

Scene::Scene(Scene &&other) noexcept
    : m_registry{std::move(other.m_registry)}
{
    m_sceneCamera = std::move(other.m_sceneCamera);
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

    m_physicsWorld = std::move(other.m_physicsWorld);
}

Scene &Scene::operator=(Scene &&other)
{
    m_registry = std::move(other.m_registry);
    m_gameObjects = std::move(other.m_gameObjects);
    m_sceneCamera = std::move(other.m_sceneCamera);
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

    // before serializing change the passed in tag
    mTag = tag;

    serialize(out);

    // restore scene tag
    mTag = sceneTag;

    YAML::Node serializedScene = YAML::Load(out.c_str());

    return Scene{std::move(serializedScene)};
}

void Scene::update()
{
    if (m_play)
    {
        std::cout << "systems: " << m_systems.size() << std::endl;
        for(auto &system : m_systems)
        {
            system->update(m_gameObjects);
        }

        // physics should be updated last as other systems make updates to it
        m_physicsWorld.simulate(Time::deltaTime(), m_gameObjects);
        m_physicsWorld.syncTransforms(m_gameObjects);

        animate();
    }
}

void Scene::play()
{
    m_play = true;
}

void Scene::pause()
{
    m_play = false;
}

void Scene::animate()
{
    auto view = m_registry.view<Animator>();
    for (auto entity : view)
    {
        auto &animator = view.get<Animator>(entity);

        // No animation selected or empty animator component
        if (animator.currentAnimation.empty())
        {
            return;
        }

        // Check if the entity has a Sprite. Create one if not present
        // TODO: should we remove this check for the actual game? Perf improvement
        if (!m_registry.any_of<Sprite>(entity))
        {
            m_registry.emplace<Sprite>(entity);
        }
        animator.update();

        auto &sprite = m_registry.get<Sprite>(entity);
        const Sprite &animatedSprite = animator.getCurrentSprite();

        bool flipX = sprite.flipX;
        bool flipY = sprite.flipY;

        sprite = animator.getCurrentSprite();

        // Restore flip values
        sprite.flipX = flipX;
        sprite.flipY = flipY;
    }
}

void Scene::registerSystem(std::unique_ptr<ISystem> system)
{
    m_systems.push_back(std::move(system));
}

void Scene::addGameObject(unsigned int width, unsigned int height, std::string &&tag)
{
    auto go = GameObject{m_registry, std::move(tag), width, height};
    m_activeEntityId = go.getEntityId();
    m_gameObjects.push_back(std::move(go));
}

void Scene::removeGameObject(entt::entity gameObject)
{
    auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
                           [&](GameObject &go)
                           { return go.getEntityId() == gameObject; });

    if(it != m_gameObjects.end())
    {
        std::cout << "game object deleted" << std::endl;
        m_gameObjects.erase(it);
    }
    m_registry.destroy(gameObject);
}

const std::vector<GameObject> &Scene::getGameObjects()
{
    return m_gameObjects;
}

SceneCamera &Scene::getCamera()
{
    return m_sceneCamera;
}

GameObject &Scene::getPlayer()
{
    for (auto &go : m_gameObjects)
    {
        if (go.getTag() == m_playerTag)
        {
            return go;
        }
    }

    throw std::runtime_error("Player not found");
}

GameObject *Scene::getActiveGameObject()
{
    // TODO: once we start having thousands of gameobjects in a single
    // scene this will become costly. Improve it.
    for (auto &go : m_gameObjects)
    {
        if (go.getEntityId() == m_activeEntityId)
        {
            return &go;
        }
    }

    return nullptr;
}

Physics2D &Scene::getPhysics2d()
{
    return m_physicsWorld;
}

void Scene::setGraivty(glm::vec2 gravity)
{
    m_physicsWorld.setGravity(gravity);
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