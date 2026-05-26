#include "core/components/BoxCollider2D.h"
#include "core/components/EntityInfo.h"
#include "core/components/Sprite.h"

#include "core/Scene.h"
#include "core/SystemRegistry.h"
#include "core/components/Animator.h"
#include "core/components/Sensor2D.h"

#include "util/LogChannels.h"
#include "util/Time.h"

#include <algorithm>
#include <chrono>
#include <entt/entt.hpp>
#include <iostream>
#include <sawyer/Logger.h>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

Scene::Scene(std::string &&tag) : mTag{tag}, m_play{false}
{
    bindRigidBodyDestroyCallback();
}

Scene::Scene(const YAML::Node &&serializedScene) : m_registry{}
{
    bindRigidBodyDestroyCallback();
    deserialize(serializedScene);
}

Scene::Scene(Scene &&other) noexcept
    : m_registry(std::move(other.m_registry)), m_physicsWorld(std::move(other.m_physicsWorld)),
      m_textures(std::move(other.m_textures)), m_systems(std::move(other.m_systems)), mTag(std::move(other.mTag)),
      m_physicsEnabled(other.m_physicsEnabled), m_animationEnabled(other.m_animationEnabled),
      m_lastPhysicsUpdateMilliseconds(other.m_lastPhysicsUpdateMilliseconds),
      m_lastAnimationUpdateMilliseconds(other.m_lastAnimationUpdateMilliseconds)
{
    bindRigidBodyDestroyCallback();
    // Move game objects while rebinding registry
    m_gameObjects.reserve(other.m_gameObjects.size());
    for (auto &g : other.m_gameObjects)
    {
        g.updateEntityReference(m_registry);
        m_gameObjects.push_back(std::move(g));
    }
    other.m_gameObjects.clear();
}

Scene &Scene::operator=(Scene &&other)
{
    if (this != &other)
    {
        m_registry = std::move(other.m_registry);
        bindRigidBodyDestroyCallback();
        m_physicsWorld = std::move(other.m_physicsWorld);
        m_textures = std::move(other.m_textures);
        m_systems = std::move(other.m_systems);
        mTag = std::move(other.mTag);
        m_physicsEnabled = other.m_physicsEnabled;
        m_animationEnabled = other.m_animationEnabled;
        m_lastPhysicsUpdateMilliseconds = other.m_lastPhysicsUpdateMilliseconds;
        m_lastAnimationUpdateMilliseconds = other.m_lastAnimationUpdateMilliseconds;

        // Move game objects while rebinding registry
        m_gameObjects.clear();
        m_gameObjects.reserve(other.m_gameObjects.size());
        for (auto &g : other.m_gameObjects)
        {
            g.updateEntityReference(m_registry);
            m_gameObjects.push_back(std::move(g));
        }
        other.m_gameObjects.clear();
    }
    return *this;
}

Scene::~Scene()
{
    for (const GameObject &go : m_gameObjects)
    {
        m_registry.destroy(go.getEntityId());
    }
    m_registry.clear();
    LOG_DEBUG(LogChannel::Scene, "Scene destructor called: {}", mTag);
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
        // In play mode, run all systems.
        for (auto &entry : m_systems)
        {
            if (!entry.enabled)
            {
                continue;
            }
            const auto start = std::chrono::steady_clock::now();
            entry.system->update(m_registry);
            entry.lastUpdateMilliseconds =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        }

        // Keep the GameObject list in sync with the registry in runtime.
        cleanupDestroyedGameObjects();

        // Advance animation before physics so frame-bound sensors match the displayed frame.
        if (m_animationEnabled)
        {
            const auto start = std::chrono::steady_clock::now();
            animate();
            m_lastAnimationUpdateMilliseconds =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        }

        // physics should be updated last as other systems make updates to it
        if (m_physicsEnabled)
        {
            const auto start = std::chrono::steady_clock::now();
            m_physicsWorld.simulate(Time::deltaTime(), m_registry);
            m_lastPhysicsUpdateMilliseconds =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        }
    }
    else
    {
        // When not playing (editor mode), run only systems that should
        // affect the editor view. For now, always run TextLayoutSystem
        // so text anchoring updates while editing.
        for (auto &entry : m_systems)
        {
            if (entry.enabled && entry.name == "TextLayoutSystem")
            {
                const auto start = std::chrono::steady_clock::now();
                entry.system->update(m_registry);
                entry.lastUpdateMilliseconds =
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            }
        }
    }
}

void Scene::cleanupDestroyedGameObjects()
{
    m_gameObjects.erase(std::remove_if(m_gameObjects.begin(),
                            m_gameObjects.end(),
                            [&](GameObject &go)
                            {
                                return !m_registry.valid(go.getEntityId());
                            }),
        m_gameObjects.end());
}

void Scene::bindRigidBodyDestroyCallback()
{
    m_registry.on_destroy<RigidBody2D>().connect<&Scene::onRigidBodyDestroyed>(this);
}

void Scene::onRigidBodyDestroyed(entt::registry &registry, entt::entity entity)
{
    m_physicsWorld.removeRigidbody(entity, registry);
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
            continue;
        }

        // Check if the entity has a Sprite. Create one if not present
        // TODO: should we remove this check for the actual game? Perf improvement
        if (!m_registry.any_of<Sprite>(entity))
        {
            m_registry.emplace<Sprite>(entity);
        }
        animator.update();

        auto &sprite = m_registry.get<Sprite>(entity);
        bool flipX = sprite.flipX;
        bool flipY = sprite.flipY;

        sprite = animator.getCurrentSprite();

        // Restore flip values
        sprite.flipX = flipX;
        sprite.flipY = flipY;

        if (m_registry.any_of<BoxCollider2D>(entity))
        {
            auto &collider = m_registry.get<BoxCollider2D>(entity);
            collider.resetAnimationState();

            for (const auto &pose : animator.getCurrentFrame().colliderShapes)
            {
                BoxColliderShape2D *shape = collider.findShape(pose.name);
                if (!shape)
                {
                    continue;
                }
                if (pose.size)
                {
                    shape->animatedSize = *pose.size;
                }
                if (pose.offset)
                {
                    shape->animatedOffset = *pose.offset;
                }
                if (pose.rotation)
                {
                    shape->animatedRotation = *pose.rotation;
                }
                if (pose.active)
                {
                    shape->animatedActive = *pose.active;
                }
            }
        }

        if (m_registry.any_of<Sensor2D>(entity))
        {
            auto &sensor = m_registry.get<Sensor2D>(entity);
            sensor.resetAnimationState();

            for (const auto &pose : animator.getCurrentFrame().sensorShapes)
            {
                SensorShape2D *shape = sensor.findShape(pose.name);
                if (!shape)
                {
                    continue;
                }
                if (pose.size)
                {
                    shape->animatedSize = *pose.size;
                }
                if (pose.offset)
                {
                    shape->animatedOffset = *pose.offset;
                }
                if (pose.active)
                {
                    shape->animatedActive = *pose.active;
                }
            }
        }
    }
}

GameObject &Scene::addGameObject(unsigned int width, unsigned int height, std::string &&tag)
{
    auto go = GameObject{
        m_registry,
    };
    // TODO: there are a lot of places where we add a component and then immediately update the values
    // it'll be better to just return the component after adding it
    go.addComponent<EntityInfo>();
    EntityInfo &entityInfo = go.getComponent<EntityInfo>();
    entityInfo.tag = std::move(tag);
    entityInfo.width = std::move(width);
    entityInfo.height = std::move(height);

    m_gameObjects.push_back(std::move(go));

    return m_gameObjects.back();
}

GameObject &Scene::addGameObjectFromModel(const std::string &modelPath)
{
    YAML::Node root;
    try
    {
        root = YAML::LoadFile(modelPath);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to load model file: " << modelPath << " error: " << e.what() << std::endl;
        throw e;
    }

    GameObject gameObj{m_registry, root, m_physicsWorld};
    m_gameObjects.push_back(std::move(gameObj));
    return m_gameObjects.back();
}

GameObject &Scene::addGameObjectFromSerialized(const YAML::Node &serializedGameObject)
{
    // FIXME: Keep this in sync with addGameObjectFromModel and
    // GameObject::GameObject(entt::registry&, const YAML::Node&, Physics2D&).
    GameObject gameObj{m_registry, serializedGameObject, m_physicsWorld};
    m_gameObjects.push_back(std::move(gameObj));
    return m_gameObjects.back();
}

void Scene::removeGameObject(entt::entity gameObject)
{
    auto it = std::find_if(m_gameObjects.begin(),
        m_gameObjects.end(),
        [&](GameObject &go)
        {
            return go.getEntityId() == gameObject;
        });

    if (it != m_gameObjects.end())
    {
        LOG_DEBUG(LogChannel::Scene, "Game object deleted");
        m_gameObjects.erase(it);
    }

    // Ensure any physics body associated with this entity is also removed
    // from the Box2D world before destroying the ECS entity.
    // TODO: Do we need this with the on_destroy<RigidBody2D> callback? It should be safe to remove this, but leaving it
    // for now as a safeguard.
    m_physicsWorld.removeRigidbody(gameObject, m_registry);
    m_registry.destroy(gameObject);
}

std::vector<GameObject> &Scene::getGameObjects()
{
    return m_gameObjects;
}

// FIXME: should be removed once we move rendering logic
// out from BaseGame.h to a rendering system
GameObject *Scene::findPrimaryCamera()
{
    for (auto &go : m_gameObjects)
    {
        if (go.hasComponent<Camera>() && go.getComponent<Camera>().primary)
        {
            return &go;
        }
    }
    return nullptr;
}

GameObject &Scene::getPlayer()
{
    for (auto &go : m_gameObjects)
    {
        EntityInfo &entityInfo = go.getComponent<EntityInfo>();
        if (entityInfo.tag == m_playerTag)
        {
            return go;
        }
    }

    throw std::runtime_error("Player not found");
}

Physics2D &Scene::getPhysics2d()
{
    return m_physicsWorld;
}

void Scene::setGravity(glm::vec2 gravity)
{
    m_physicsWorld.setGravity(gravity);
}

const std::string &Scene::getTag() const
{
    return mTag;
}

void Scene::addSystem(const std::string &systemName)
{
    const auto it = std::find_if(m_systems.begin(),
        m_systems.end(),
        [&](const SystemEntry &entry)
        {
            return entry.name == systemName;
        });
    if (it != m_systems.end())
    {
        return;
    }

    std::shared_ptr<ISystem> system = SystemRegistry::instance().get(systemName);
    m_systems.push_back({systemName, std::move(system)});
}

std::vector<Scene::SystemInfo> Scene::getSystemInfo() const
{
    std::vector<SystemInfo> result;
    result.reserve(m_systems.size());
    for (const auto &entry : m_systems)
    {
        result.push_back({entry.name, entry.enabled, entry.name == "TextLayoutSystem", entry.lastUpdateMilliseconds});
    }
    return result;
}

bool Scene::setSystemEnabled(const std::string &systemName, bool enabled)
{
    const auto it = std::find_if(m_systems.begin(),
        m_systems.end(),
        [&](const SystemEntry &entry)
        {
            return entry.name == systemName;
        });
    if (it == m_systems.end())
    {
        return false;
    }
    if (it->enabled == enabled)
    {
        return true;
    }
    it->enabled = enabled;
    if (enabled)
    {
        it->system->onEnabled(m_registry);
    }
    else
    {
        it->system->onDisabled(m_registry);
    }
    return true;
}

void Scene::setAllSystemsEnabled(bool enabled)
{
    for (const auto &entry : m_systems)
    {
        setSystemEnabled(entry.name, enabled);
    }
}

bool Scene::isPhysicsEnabled() const
{
    return m_physicsEnabled;
}

void Scene::setPhysicsEnabled(bool enabled)
{
    m_physicsEnabled = enabled;
}

double Scene::getLastPhysicsUpdateMilliseconds() const
{
    return m_lastPhysicsUpdateMilliseconds;
}

bool Scene::isAnimationEnabled() const
{
    return m_animationEnabled;
}

void Scene::setAnimationEnabled(bool enabled)
{
    m_animationEnabled = enabled;
}

double Scene::getLastAnimationUpdateMilliseconds() const
{
    return m_lastAnimationUpdateMilliseconds;
}

bool Scene::serialize(YAML::Emitter &out)
{
    out << YAML::BeginMap;
    out << YAML::Key << mTag;
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "GameObjects";
    out << YAML::Value << YAML::BeginSeq;

    for (GameObject &gameObject : m_gameObjects)
    {
        gameObject.serialize(out);
    }

    out << YAML::EndSeq;

    out << YAML::Key << "Systems" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (const auto &entry : m_systems)
    {
        out << entry.name;
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
    out << YAML::EndMap;

    // TODO: Do we need this to return bool?
    return true;
}

void Scene::deserialize(const YAML::Node &in)
{
    for (YAML::const_iterator it = in.begin(); it != in.end(); ++it)
    {
        LOG_INFO(LogChannel::Scene, "Deserializing scene: {}", it->first.as<std::string>());
        mTag = it->first.as<std::string>();

        // Deserializing Systems
        // FIXME: Systems should be deserialized first as there might be game object
        // components that depend on system initialization. An example is Audio
        // System that initializes, OpenAL context, without which AudioSource
        // component won't work. Ideally, we would want to avoid dependencies
        // like this. Refactor.
        if (it->second["Systems"])
        {
            YAML::Node systems = it->second["Systems"];
            for (const auto &system : systems)
            {
                addSystem(system.as<std::string>());
            }
        }

        // Deserializing Game Objects
        YAML::Node gameObjects = it->second["GameObjects"];
        for (const auto &serializedGameObject : gameObjects)
        {
            GameObject gameObj{m_registry, serializedGameObject, m_physicsWorld};
            m_gameObjects.push_back(std::move(gameObj));
        }
    }
}
