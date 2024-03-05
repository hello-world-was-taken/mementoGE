#pragma once
#include <entt/entt.hpp>
#include <iostream>
#include <memory>

class GameObject
{
private:
    entt::entity entity;
    entt::registry &registry;

public:
    GameObject(entt::registry &registry);
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

    // TODO: Debug function, remove later
    void getId()
    {
        std::cout << "Entity ID: " << (u_int32_t)entity << std::endl;
    }
};