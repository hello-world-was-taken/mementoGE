#include "core/GameObject.h"
#include "core/Transform.h"
#include "core/Sprite.h"
#include "core/RigidBody2D.h"

#include "physics/Physics2D.h"

#include <yaml-cpp/yaml.h>
#include <string>

GameObject::GameObject(entt::registry &registry, std::string &&tag, unsigned int width, unsigned int height) : m_registry{&registry}, mTag{tag}, m_width{width}, m_height{height}
{
    m_entity = m_registry->create();
    addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f));
}

GameObject::GameObject(entt::registry &registry, const YAML::Node &serializedGameObject) : m_registry{&registry}
{
    m_entity = m_registry->create();

    // Deserializing
    mTag = serializedGameObject["Tag"].as<std::string>();
    m_width = serializedGameObject["Width"].as<unsigned int>();
    m_height = serializedGameObject["Height"].as<unsigned int>();

    // Deserializing Transform Component of the Game Object
    addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f));
    getComponent<Transform>().deserialize(serializedGameObject);

    // Deserialize Sprite Component of the Game Object
    addComponent<Sprite>();
    getComponent<Sprite>().deserialize(serializedGameObject);
}

GameObject::GameObject(entt::registry &registry, const YAML::Node &serializedGameObject, Physics2D &physics) : m_registry{&registry}
{
    m_entity = m_registry->create();

    // Deserializing
    mTag = serializedGameObject["Tag"].as<std::string>();
    m_width = serializedGameObject["Width"].as<unsigned int>();
    m_height = serializedGameObject["Height"].as<unsigned int>();

    // Deserializing Transform Component of the Game Object
    addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f));
    getComponent<Transform>().deserialize(serializedGameObject);

    // Deserialize Sprite Component of the Game Object
    addComponent<Sprite>();
    getComponent<Sprite>().deserialize(serializedGameObject);

    bool hasRigidBody2D = false;
    if (serializedGameObject["HasRigidBody2D"] && serializedGameObject["HasRigidBody2D"].IsScalar())
    {
        hasRigidBody2D = serializedGameObject["HasRigidBody2D"].as<bool>();
        if (hasRigidBody2D)
        {
            physics.addRigidbody(*this);
        }
    }
}

GameObject::GameObject(GameObject &&other)
    : m_registry{other.m_registry}
{
    m_entity = other.m_entity;
    m_width = other.m_width;
    m_height = other.m_height;
    mTag = std::move(other.mTag);
}

GameObject &GameObject::operator=(GameObject &&other)
{
    m_registry = other.m_registry;
    m_entity = other.m_entity;
    m_width = other.m_width;
    m_height = other.m_height;
    mTag = std::move(other.mTag);

    return *this;
}

GameObject::~GameObject()
{
    // m_registry.destroy(m_entity);
}

void GameObject::destroy()
{
    // m_registry.destroy(m_entity);
}

void GameObject::setWidth(int width)
{
    m_width = width;
}

void GameObject::setHeight(int height)
{
    m_height = height;
}

const int GameObject::getWidth() const
{
    return m_width;
}

const int GameObject::getHeight() const
{
    return m_height;
}

const std::string &GameObject::getTag() const
{
    return mTag;
}

std::vector<glm::vec3> GameObject::getQuad() const
{
    return std::vector<glm::vec3>({{0.0f, 1.0f * m_height, 0.0f},             // top left
                                   {0.0f, 0.0f, 0.0f},                        // bottom left
                                   {1.0f * m_width, 0.0f, 0.0f},              // bottom right
                                   {1.0f * m_width, 1.0f * m_height, 0.0f}}); // top right
}

// TODO: shouldn't this be done in the GPU? It shouldn't matter that much for 2D, but as a principle.
// Also is it possible to do that if we want to draw the batch in a single draw call?
std::vector<glm::vec3> GameObject::getWorldCoordinateQuad() const
{
    Transform transform = getComponent<Transform>();
    glm::mat4x4 modelMatrix = transform.getModelMatrix();
    std::vector<glm::vec3> quad = getQuad();
    std::vector<glm::vec3> transformedQuad = quad;
    for (int i = 0; i < quad.size(); i++)
    {
        transformedQuad[i] = modelMatrix * glm::vec4(quad[i], 1.0f);
    }

    return transformedQuad;
}

void GameObject::updateEntityReference(entt::registry &registry)
{
    m_registry = &registry;
}

// TODO: we can update this to use spatial grid or quadtree as an optimization
bool GameObject::containsPoint(glm::vec2 worldPoint) const
{
    std::vector<glm::vec3> worldQuad = getWorldCoordinateQuad();
    glm::vec2 topLeft = worldQuad[0];
    return topLeft.x <= worldPoint.x && worldPoint.x <= topLeft.x + m_width &&
           topLeft.y >= worldPoint.y && worldPoint.y >= topLeft.y - m_height;
}

bool GameObject::serialize(YAML::Emitter &out)
{
    out << YAML::Key << mTag;
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "Tag";
    out << YAML::Value << mTag;
    out << YAML::Key << "Width";
    out << YAML::Value << m_width;
    out << YAML::Key << "Height";
    out << YAML::Value << m_height;

    // TRANSFORM COMPONENT
    if (hasComponent<Transform>())
    {
        Transform &transform = getComponent<Transform>();
        transform.serialize(out);
    }

    // SPRITE COMPONENT
    if (hasComponent<Sprite>())
    {
        Sprite &sprite = getComponent<Sprite>();
        sprite.serialize(out);
    }

    // RIGIDBODY2D COMPONENT
    out << YAML::Key << "HasRigidBody2D";
    out << YAML::Value << hasComponent<Rigidbody2D>();

    out << YAML::EndMap;

    // TODO: do we need this to return a bool?
    return true;
}
