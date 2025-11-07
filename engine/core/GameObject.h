#pragma once

#include "core/components/BoxCollider2D.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Transform.h"
#include "core/components/EnemyState.h"

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

// TODO: can we make gameObject a light wrapper around entity?
// maybe move the utilities to the scene?
class GameObject
{
public:
    GameObject(entt::registry &registry);
    GameObject(entt::registry &registry, const YAML::Node &serializedGameObject, Physics2D &physics);
    GameObject(GameObject &&other);
    GameObject &operator=(GameObject &&other);
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

    void destroy();

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
    m_registry->emplace<Component>(m_entity, std::forward<Args>(args)...);
}

template <typename Component> bool GameObject::hasComponent() const
{
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