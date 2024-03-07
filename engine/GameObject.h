#pragma once
#include <entt/entt.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class GameObject
{
private:
    entt::entity entity;
    entt::registry &registry;
    unsigned int m_width = 0;
    unsigned int m_height = 0;

public:
    GameObject(entt::registry &registry, unsigned int width = 0, unsigned int height = 0);
    ~GameObject();

    template <typename Component, typename... Args>
    void addComponent(Args &&...args)
    {
        registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template <typename Component>
    bool hasComponent() const
    {
        return registry.all_of<Component>(entity);
    }

    template <typename Component>
    Component &getComponent()
    {
        return registry.get<Component>(entity);
    }

    template <typename Component>
    void removeComponent()
    {
        registry.remove<Component>(entity);
    }

    void destroy();

    unsigned int getWidth();

    unsigned int getHeight();

    std::vector<glm::vec3> getQuad();

    // TODO: Debug function, remove later
    void getId()
    {
        std::cout << "Entity ID: " << (u_int32_t)entity << std::endl;
    }
};