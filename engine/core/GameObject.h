#pragma once

#include "core/components/BoxCollider2D.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Transform.h"
#include "core/components/EnemyState.h"
#include "core/components/EntityInfo.h"

#include "physics/Physics2D.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <yaml-cpp/yaml.h>

/**
 * @brief A lightweight, non-owning wrapper around an entt::entity.
 *
 * GameObject does NOT own the underlying entity or any of its components.
 * The Scene (and its entt::registry) is the sole owner.
 *
 * This class is designed to be:
 *  - Copyable: copying creates another handle to the same entity.
 *  - Movable: moving transfers the handle but does not affect the entity.
 *  - Safe: a moved-from GameObject becomes an empty/invalid handle.
 *
 * GameObject is only a convenience layer to access components,
 * perform serialization, and provide higher-level helper utilities.
 */
class GameObject
{
public:
    GameObject(entt::registry &registry);
    GameObject(entt::registry &registry, const YAML::Node &serializedGameObject, Physics2D &physics);
    ~GameObject() = default;

    template <typename Component, typename... Args> void addComponent(Args &&...args);
    template <typename Component> bool hasComponent() const;
    template <typename Component> Component &getComponent() const;
    template <typename Component> void removeComponent();
    template <typename Component> void serializeComponent(YAML::Emitter &out);
    template <typename Component>
    void deserializeComponent(const YAML::Node &serializedGameObject,
        const std::string &componentName,
        std::optional<std::reference_wrapper<Physics2D>> physics = std::nullopt);

    std::array<glm::vec3, 4> getQuad() const;
    std::array<glm::vec3, 4> getWorldCoordinateQuad() const;

    // mouse press detection
    bool containsPoint(glm::vec2 worldPoint) const;

    entt::entity getEntityId() const
    {
        // std::cout << "Entity ID: " << entt::to_entity(m_entity) << " Version: " << entt::to_version(m_entity) <<
        // std::endl;
        return m_entity;
    }

    void updateEntityReference(entt::registry &registry);

    bool serialize(YAML::Emitter &out);

private:
    // We take registry as a reference and internally use it as a pointer to support
    // assignment operation
    entt::registry *m_registry;
    entt::entity m_entity;
};

template <typename Component, typename... Args> void GameObject::addComponent(Args &&...args)
{
    // Check if the entity already has this component
    if (m_registry->any_of<Component>(m_entity))
    {
        std::cerr << "[GameObject] addComponent failed: entity " << static_cast<entt::id_type>(m_entity)
                  << " already has component " << typeid(Component).name() << "\n";
        return;
    }

    m_registry->emplace<Component>(m_entity, std::forward<Args>(args)...);
}

template <typename Component> bool GameObject::hasComponent() const
{
    // std::cout << "has component called for" << typeid(Component).name() << " registry destroyed: " << (m_registry == nullptr) << std::endl;
    // auto &test = m_registry->get<EntityInfo>(m_entity);
    // std::cout << "entity is valid: " << m_registry->valid(m_entity) << std::endl;
    // std::cout << "entity name: " << test.tag << std::endl;
    return m_registry->all_of<Component>(m_entity);
}

template <typename Component> Component &GameObject::getComponent() const
{
    if (!hasComponent<Component>())
    {
        // TODO: Use the demangled name of the type using -> abi::__cxa_demangle from cxxabi.h
        //       after setting up a proper logging system.
        std::cerr << "ERROR: Component " << typeid(Component).name() << " does not exist for the Game Object"
                  << std::endl;
        throw std::runtime_error("Component does not exist");
    }
    return m_registry->get<Component>(m_entity);
}

template <typename Component> void GameObject::removeComponent()
{
    m_registry->remove<Component>(m_entity);
}

template <typename Component> void GameObject::serializeComponent(YAML::Emitter &out)
{
    if (hasComponent<Component>())
    {
        // TODO: add static assert to make sure serialize exists
        Component &component = getComponent<Component>();
        component.serialize(out);
    }
}

template <typename Component>
void GameObject::deserializeComponent(const YAML::Node &serializedGameObject,
    const std::string &componentName,
    std::optional<std::reference_wrapper<Physics2D>> physics)
{
    if (serializedGameObject[componentName])
    {
        addComponent<Component>();
        getComponent<Component>().deserialize(serializedGameObject);

        if constexpr (std::is_same_v<Component, RigidBody2D>)
        {
            if (physics)
            {
                physics->get().registerRigidBody2D(*this);
            }
        }
        else if constexpr (std::is_same_v<Component, BoxCollider2D>)
        {
            if (physics)
            {
                physics->get().registerBoxCollider2D(*this);
            }
        }
        else if constexpr (std::is_same_v<Component, Sensor2D>)
        {
            if (physics)
            {
                physics->get().registerSensor2D(*this);
            }
        }
        else if constexpr (std::is_same_v<Component, EnemyState>)
        {
            getComponent<EnemyState>().startPosition = glm::vec3{getComponent<Transform>().position};
        }
    }
}