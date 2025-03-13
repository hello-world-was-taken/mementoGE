#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <typeinfo>
#include <type_traits>
#include <yaml-cpp/yaml.h>

class GameObject
{
public:
    GameObject(entt::registry &registry, std::string &&tag, unsigned int width = 0, unsigned int height = 0);
    GameObject(entt::registry &registry, const YAML::Node &serializedGameObject);
    GameObject(GameObject &&other);
    GameObject &operator=(GameObject &&other);
    ~GameObject();

    template <typename Component, typename... Args>
    void addComponent(Args &&...args)
    {
        m_registry->emplace<Component>(m_entity, std::forward<Args>(args)...);
    }

    template <typename Component>
    bool hasComponent() const
    {
        return m_registry->all_of<Component>(m_entity);
    }

    template <typename Component>
    Component &getComponent()
    {
        if (!hasComponent<Component>())
        {
            // TODO: Use the demangled name of the type using -> abi::__cxa_demangle from cxxabi.h
            //       after setting up a proper logging system.
            std::cerr << "ERROR: Component " << typeid(Component).name() << " does not exist for the Game Object" << std::endl;
            throw std::runtime_error("Component does not exist");
        }
        return m_registry->get<Component>(m_entity);
    }

    template <typename Component>
    void removeComponent()
    {
        m_registry->remove<Component>(m_entity);
    }

    void destroy();

    const int getWidth() const;

    const int getHeight() const;

    const std::string &getTag() const;

    // TODO: why are we returning a copy here?
    std::vector<glm::vec3> getQuad();

    // TODO: Debug function, remove later
    void getId() const
    {
        std::cout << "Entity ID: " << entt::to_entity(m_entity) << " Version: " << entt::to_version(m_entity) << std::endl;
    }

    void updateEntityReference(entt::registry &registry);

    bool serialize(YAML::Emitter &out);

    // TODO: we should add a constructor that takes in serialized gameobject YAML::Node.
    entt::entity m_entity;

private:
    // TODO: we should change this to unique_ptr
    // We take registry as a reference and internally use it as a pointer to support
    // assignment operation
    entt::registry *m_registry;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    std::string mTag; // human readable name of game object
};