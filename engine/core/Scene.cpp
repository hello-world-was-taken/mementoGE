#include "core/components/EntityInfo.h"
#include "core/components/Sprite.h"

#include "core/Scene.h"
#include "core/SystemRegistry.h"
#include "core/components/Animator.h"

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

Scene::Scene(Scene &&other) noexcept
    : m_registry(std::move(other.m_registry)), m_sceneCamera(std::move(other.m_sceneCamera)),
      m_physicsWorld(std::move(other.m_physicsWorld)), m_textures(std::move(other.m_textures)),
      m_systemNames(std::move(other.m_systemNames)), m_systems(std::move(other.m_systems)), mTag(std::move(other.mTag))
{
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
        m_sceneCamera = std::move(other.m_sceneCamera);
        m_physicsWorld = std::move(other.m_physicsWorld);
        m_textures = std::move(other.m_textures);
        m_systemNames = std::move(other.m_systemNames);
        m_systems = std::move(other.m_systems);
        mTag = std::move(other.mTag);

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
        // In play mode, run all systems.
        for (auto &system : m_systems)
        {
            system->update(m_registry);
        }

        // physics should be updated last as other systems make updates to it
        m_physicsWorld.simulate(Time::deltaTime(), m_registry);

        animate();
    }
    else
    {
        // When not playing (editor mode), run only systems that should
        // affect the editor view. For now, always run TextLayoutSystem
        // so text anchoring updates while editing.
        for (std::size_t i = 0; i < m_systems.size(); ++i)
        {
            if (m_systemNames[i] == "TextLayoutSystem")
            {
                m_systems[i]->update(m_registry);
            }
        }
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
    auto it = std::find(m_systemNames.begin(), m_systemNames.end(), systemName);
    if (it != m_systemNames.end())
    {
        return;
    }

    std::shared_ptr<ISystem> system = SystemRegistry::instance().get(systemName);
    m_systems.push_back(system);
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
        for (const auto &it : gameObjects)
        {
            GameObject gameObj{m_registry, it.second, m_physicsWorld};
            m_gameObjects.push_back(std::move(gameObj));
        }
    }
}