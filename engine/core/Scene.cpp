#include <iostream>
#include <string>
#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>

#include "core/Scene.h"
#include "renderer/RenderBatch.h"
#include "core/MouseListener.h" // TODO: Game specific things shouldn't be here.

Scene::Scene(std::string &&tag)
    : mTag{tag}
{
    // NOTE: When trying to construct RenderBatch here, it was causing a segfault. I think it was because
    // the OpenGL context was not yet created which resulted in glGenVertexArrays == NULL. So, I moved the
    // RenderBatch construction to the start.
    // TODO: Investigate this further
    // m_renderBatch = new RenderBatch(this);
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
            GameObject gameObj{m_registry, it->second};
            m_gameObjects.push_back(std::move(gameObj));
        }

        // TODO: FIX THIS!
        m_activeGameObject = &m_gameObjects.back();
    }
}

Scene::Scene(Scene &&other)
    : m_registry{std::move(other.m_registry)}
{
    m_screen_height = other.m_screen_height;
    m_screen_width = other.m_screen_width;
    m_camera = std::move(other.m_camera);
    m_renderBatch = other.m_renderBatch;
    m_textures = std::move(other.m_textures);
    mTag = std::move(other.mTag);

    for (GameObject &g : other.m_gameObjects)
    {
        g.updateEntityReference(m_registry);
        m_gameObjects.push_back(std::move(g));
    }

    // setting other pointer variables to null_ptr
    other.m_renderBatch = nullptr;
    other.m_activeGameObject = nullptr;
    other.m_registry = NULL;

    // TODO: DO THIS IN A BETTER WAY
    if (m_gameObjects.size())
        m_activeGameObject = &m_gameObjects.back();
}

Scene &Scene::operator=(Scene &&other)
{
    m_screen_height = other.m_screen_height;
    m_screen_width = other.m_screen_width;
    m_registry = std::move(other.m_registry);
    m_gameObjects = std::move(other.m_gameObjects);
    m_camera = std::move(other.m_camera);
    m_renderBatch = other.m_renderBatch;
    m_textures = std::move(other.m_textures);
    m_activeGameObject = other.m_activeGameObject;
    mTag = std::move(other.mTag);

    // setting other pointer variables to null_ptr
    other.m_renderBatch = nullptr;
    other.m_activeGameObject = nullptr;

    return *this;
}

Scene::~Scene()
{
    std::cout << "Scene destructor called: " << mTag << std::endl;
    delete m_renderBatch;
}

void Scene::start()
{
    m_renderBatch = new RenderBatch(m_camera, m_gameObjects);
}

void Scene::update(float deltaTime, GLFWwindow *window)
{
    m_renderBatch->render();

    MouseListener *listener = MouseListener::getListener();
    listener->getWorldCoordinates(m_camera);
}

GameObject &Scene::addGameObject(unsigned int width, unsigned int height, std::string &&tag)
{
    m_gameObjects.push_back(GameObject{m_registry, std::move(tag), width, height});
    // TODO: FIX THIS
    // Since m_gameObjects is a vector, m_activeGameObject may point
    // to invalid address when our vector gets resized.
    // Better to use idx of the active game object
    m_activeGameObject = &m_gameObjects.back();
    return m_gameObjects.back();
}

std::vector<GameObject> &Scene::getGameObjects()
{
    return m_gameObjects;
}

std::shared_ptr<Camera> Scene::getCamera() const
{
    return m_camera;
}

// TODO: If no game object is present, it should throw
GameObject &Scene::getActiveGameObject()
{
    return m_gameObjects.back();
}

void Scene::setActiveGameObject(GameObject *gameObject)
{
    m_activeGameObject = gameObject;
}

const std::string &Scene::getTag() const
{
    return mTag;
}

bool Scene::serialize(YAML::Emitter &out)
{
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

    // TODO: Do we need this to return bool?
    return true;
}