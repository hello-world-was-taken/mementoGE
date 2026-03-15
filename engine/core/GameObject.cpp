#include "core/components/AudioSource.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/Camera.h"
#include "core/components/EnemyState.h"
#include "core/components/EntityInfo.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/Patrol.h"
#include "core/components/PostProcessSettings.h"
#include "core/components/RenderLayer.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"
#include "core/components/TextAnchor.h"
#include "core/components/Transform.h"

#include "core/ComponentRegistry.h"
#include "core/GameObject.h"
#include "core/components/Animator.h"

#include "physics/Physics2D.h"

#include <string>
#include <yaml-cpp/yaml.h>

GameObject::GameObject(entt::registry &registry) : m_registry{&registry}
{
    m_entity = m_registry->create();
    addComponent<EntityInfo>();
    addComponent<Transform>();
    addComponent<RenderLayer>();
}

GameObject::GameObject(entt::registry &registry, const YAML::Node &serializedGameObject, Physics2D &physics)
    : m_registry{&registry}
{
    m_entity = m_registry->create();
    // add required components to objects that were created previously
    // and may not have them for backwards compatablity
    addComponent<EntityInfo>();
    addComponent<Transform>();
    addComponent<RenderLayer>();

    // Deserialize Components
    deserializeComponent<EntityInfo>(serializedGameObject, "EntityInfo");
    deserializeComponent<Transform>(serializedGameObject, "Transform");
    deserializeComponent<RenderLayer>(serializedGameObject, "RenderLayer");
    deserializeComponent<Sprite>(serializedGameObject, "Sprite");
    deserializeComponent<RigidBody2D>(serializedGameObject, "RigidBody2D", physics);
    deserializeComponent<BoxCollider2D>(serializedGameObject, "BoxCollider2D", physics);
    deserializeComponent<Sensor2D>(serializedGameObject, "Sensor2D", physics);
    deserializeComponent<Animator>(serializedGameObject, "Animator");
    deserializeComponent<AudioSource>(serializedGameObject, "AudioSource");
    deserializeComponent<EnemyState>(serializedGameObject, "EnemyState");
    deserializeComponent<Patrol>(serializedGameObject, "Patrol");
    deserializeComponent<Text>(serializedGameObject, "Text");
    deserializeComponent<TextAnchor>(serializedGameObject, "TextAnchor");
    deserializeComponent<ParticleEmitter>(serializedGameObject, "ParticleEmitter");
    deserializeComponent<Camera>(serializedGameObject, "Camera");
    deserializeComponent<PostProcessSettings>(serializedGameObject, "PostProcessSettings");

    // Let game code plug in additional component deserialization.
    const auto &extraEntries = ComponentRegistry::instance().getEntries();
    for (const auto &entry : extraEntries)
    {
        if (entry.deserialize)
        {
            entry.deserialize(*this, serializedGameObject);
        }
    }
}

std::array<glm::vec3, 4> GameObject::getQuad() const
{
    EntityInfo &entityInfo = getComponent<EntityInfo>();

    float halfWidth = entityInfo.width * 0.5f;
    float halfHeight = entityInfo.height * 0.5f;

    glm::vec3 topLeft = {-halfWidth, halfHeight, 0.0f};
    glm::vec3 bottomLeft = {-halfWidth, -halfHeight, 0.0f};
    glm::vec3 bottomRight = {halfWidth, -halfHeight, 0.0f};
    glm::vec3 topRight = {halfWidth, halfHeight, 0.0f};

    return std::array{topLeft, bottomLeft, bottomRight, topRight};
}

// TODO: shouldn't this be done in the GPU? It shouldn't matter that
// much for 2D, but as a principle. Also is it possible to do that if
// we want to draw the batch in a single draw call?
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

// TODO: we can update this to use spatial grid or quadtree as an
// optimization
bool GameObject::containsPoint(glm::vec2 worldPoint) const
{
    EntityInfo &entityInfo = getComponent<EntityInfo>();
    std::array<glm::vec3, 4> worldQuad = getWorldCoordinateQuad();
    glm::vec2 topLeft = worldQuad[0];
    return topLeft.x <= worldPoint.x && worldPoint.x <= topLeft.x + entityInfo.width && topLeft.y >= worldPoint.y &&
           worldPoint.y >= topLeft.y - entityInfo.height;
}

bool GameObject::serialize(YAML::Emitter &out)
{
    EntityInfo &entityInfo = getComponent<EntityInfo>();
    out << YAML::Key << entityInfo.tag;
    out << YAML::Value << YAML::BeginMap;

    serializeComponent<EntityInfo>(out);
    serializeComponent<Transform>(out);
    serializeComponent<RenderLayer>(out);
    serializeComponent<Sprite>(out);
    serializeComponent<RigidBody2D>(out);
    serializeComponent<BoxCollider2D>(out);
    serializeComponent<Sensor2D>(out);
    serializeComponent<Animator>(out);
    serializeComponent<EnemyState>(out);
    serializeComponent<Patrol>(out);
    serializeComponent<Text>(out);
    serializeComponent<TextAnchor>(out);
    serializeComponent<ParticleEmitter>(out);
    serializeComponent<Camera>(out);
    serializeComponent<PostProcessSettings>(out);

    // Let game code plug in additional component serialization.
    const auto &extraEntries = ComponentRegistry::instance().getEntries();
    for (const auto &entry : extraEntries)
    {
        if (entry.serialize)
        {
            entry.serialize(*this, out);
        }
    }

    out << YAML::EndMap;

    // TODO: do we need this to return a bool?
    return true;
}
