#include "core/components/Patrol.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sprite.h"
#include "core/components/Transform.h"
#include "core/components/EnemyState.h"

#include "core/Animator.h"
#include "core/GameObject.h"

#include "physics/Physics2D.h"

#include <string>
#include <yaml-cpp/yaml.h>

GameObject::GameObject(entt::registry &registry, std::string &&tag, unsigned int width, unsigned int height)
    : m_registry{&registry}, mTag{tag}, m_width{width}, m_height{height}
{
    m_entity = m_registry->create();
    addComponent<Transform>();
}

GameObject::GameObject(entt::registry &registry, const YAML::Node &serializedGameObject, Physics2D &physics)
    : m_registry{&registry}
{
    m_entity = m_registry->create();

    // Deserializing
    mTag = serializedGameObject["Tag"].as<std::string>();
    m_width = serializedGameObject["Width"].as<unsigned int>();
    m_height = serializedGameObject["Height"].as<unsigned int>();

    // Deserialize Components
    deserializeComponent<Transform>(serializedGameObject, "Transform");
    deserializeComponent<Sprite>(serializedGameObject, "Sprite");
    deserializeComponent<BoxCollider2D>(serializedGameObject, "BoxCollider2D");
    deserializeComponent<RigidBody2D>(serializedGameObject, "RigidBody2D", physics);
    deserializeComponent<Animator>(serializedGameObject, "Animator");
    deserializeComponent<EnemyState>(serializedGameObject, "EnemyState");
    deserializeComponent<Patrol>(serializedGameObject, "Patrol");
}

GameObject::GameObject(GameObject &&other) : m_registry{other.m_registry}
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

void GameObject::setTag(const std::string &newTag)
{
    mTag = newTag;
}

const std::string &GameObject::getTag() const
{
    return mTag;
}

std::array<glm::vec3, 4> GameObject::getQuad() const
{
    float halfWidth = m_width * 0.5f;
    float halfHeight = m_height * 0.5f;

    glm::vec3 topLeft = {-halfWidth, halfHeight, 0.0f};
    glm::vec3 bottomLeft = {-halfWidth, -halfHeight, 0.0f};
    glm::vec3 bottomRight = {halfWidth, -halfHeight, 0.0f};
    glm::vec3 topRight = {halfWidth, halfHeight, 0.0f};

    return std::array{topLeft, bottomLeft, bottomRight, topRight};
}

// TODO: shouldn't this be done in the GPU? It shouldn't matter that much for 2D, but as a principle.
// Also is it possible to do that if we want to draw the batch in a single draw call?
std::array<glm::vec3, 4> GameObject::getWorldCoordinateQuad() const
{
    Transform transform = getComponent<Transform>();
    glm::mat4x4 modelMatrix = transform.getModelMatrix();

    std::array<glm::vec3, 4> quad = getQuad();
    std::array<glm::vec3, 4> transformedQuad = quad;

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
    std::array<glm::vec3, 4> worldQuad = getWorldCoordinateQuad();
    glm::vec2 topLeft = worldQuad[0];
    return topLeft.x <= worldPoint.x && worldPoint.x <= topLeft.x + m_width && topLeft.y >= worldPoint.y &&
           worldPoint.y >= topLeft.y - m_height;
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

    serializeComponent<Transform>(out);
    serializeComponent<Sprite>(out);
    serializeComponent<RigidBody2D>(out);
    serializeComponent<BoxCollider2D>(out);
    serializeComponent<Animator>(out);
    serializeComponent<Animator>(out);
    serializeComponent<EnemyState>(out);
    serializeComponent<Patrol>(out);

    out << YAML::EndMap;

    // TODO: do we need this to return a bool?
    return true;
}
