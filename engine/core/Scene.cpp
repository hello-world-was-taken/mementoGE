#include "core/Scene.h"
#include "core/Animator.h"
#include "core/SystemRegistry.h"
#include "core/components/Sprite.h"

#include "util/Time.h"

#include <algorithm>
#include <entt/entt.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

Scene::Scene(std::string &&tag) : mTag{tag}, m_play{false}
{
}

Scene::Scene(const YAML::Node &&serializedScene) : m_registry{}
{
    deserialize(serializedScene);
}

Scene::Scene(Scene &&other) noexcept : m_registry{std::move(other.m_registry)}
{
    m_sceneCamera = std::move(other.m_sceneCamera);
    m_textures = std::move(other.m_textures);
    m_systemNames = std::move(other.m_systemNames);
    m_systems = std::move(other.m_systems);

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
    m_systemNames = std::move(other.m_systemNames);
    m_systems = std::move(other.m_systems);
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
        for (auto &system : m_systems)
        {
            system->update(m_gameObjects);
        }

        // physics should be updated last as other systems make updates to it
        m_physicsWorld.simulate(Time::deltaTime(), m_gameObjects);

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

GameObject &Scene::addGameObject(unsigned int width, unsigned int height, std::string &&tag)
{
    auto go = GameObject{m_registry, std::move(tag), width, height};
    m_activeEntityId = go.getEntityId();
    m_gameObjects.push_back(std::move(go));

    return m_gameObjects.back();
}

void Scene::removeGameObject(entt::entity gameObject)
{
    auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
        [&](GameObject &go)
        {
            return go.getEntityId() == gameObject;
        });

    if (it != m_gameObjects.end())
    {
        std::cout << "game object deleted" << std::endl;
        m_gameObjects.erase(it);
    }
    m_registry.destroy(gameObject);
}

std::vector<GameObject> &Scene::getGameObjects()
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

Physics2D &Scene::getPhysics2d()
{
    return m_physicsWorld;
}

void Scene::setGraivty(glm::vec2 gravity)
{
    m_physicsWorld.setGravity(gravity);
}


const std::string &Scene::getTag() const
{
    return mTag;
}

void Scene::addSystem(const std::string &systemName)
{
    auto it = std::find(m_systemNames.begin(), m_systemNames.end(), systemName);
    if (it != m_systemNames.end())
    {
        return;
    }

    std::unique_ptr<ISystem> system = SystemRegistry::instance().create(systemName)(); // create returns a lamda
    m_systems.push_back(std::unique_ptr<ISystem>(system.release()));
    m_systemNames.push_back(systemName);
}

bool Scene::serialize(YAML::Emitter &out)
{
    out << YAML::BeginMap;
    out << YAML::Key << mTag;
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "GameObjects";
    out << YAML::Value << YAML::BeginMap;

    for (GameObject &gameObject : m_gameObjects)
    {
        gameObject.serialize(out);
    }

    out << YAML::EndMap;

    out << YAML::Key << "Systems" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (const auto &systemName : m_systemNames)
    {
        out << systemName;
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
        std::cout << "Deserializing Scene: " << it->first.as<std::string>() << std::endl;
        mTag = it->first.as<std::string>();

        // Deserializing Game Objects
        YAML::Node gameObjects = it->second["GameObjects"];
        for (const auto &it : gameObjects)
        {
            GameObject gameObj{m_registry, it.second, m_physicsWorld};
            m_gameObjects.push_back(std::move(gameObj));
        }

        // Deserializing Systems
        if (it->second["Systems"])
        {
            YAML::Node systems = it->second["Systems"];
            for (const auto &system : systems)
            {
                addSystem(system.as<std::string>());
            }
        }

        // TODO: FIX THIS! Exception is thrown when scene.yaml doesn't have any objects
        m_activeEntityId = m_gameObjects.back().getEntityId();
    }
}