#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <typeinfo>
#include <type_traits>

class GameObject
{
private:
    entt::entity m_entity;
    entt::registry &m_registry;
    int m_width = 0;
    int m_height = 0;

public:
    GameObject(entt::registry &registry, unsigned int width = 0, unsigned int height = 0);
    ~GameObject();

    template <typename Component, typename... Args>
    void addComponent(Args &&...args)
    {
        m_registry.emplace<Component>(m_entity, std::forward<Args>(args)...);
    }

    template <typename Component>
    bool hasComponent() const
    {
        return m_registry.all_of<Component>(m_entity);
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
        return m_registry.get<Component>(m_entity);
    }

    template <typename Component>
    void removeComponent()
    {
        m_registry.remove<Component>(m_entity);
    }

    void destroy();

    int& getWidth();

    int& getHeight();

    std::vector<glm::vec3> getQuad();

    // TODO: Debug function, remove later
    void getId()
    {
        std::cout << "Entity ID: " << (u_int32_t)m_entity << std::endl;
    }
};