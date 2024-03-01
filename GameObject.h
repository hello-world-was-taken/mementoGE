#include <entt/entt.hpp>
#include <iostream>

class GameObject
{
public:
    GameObject(entt::registry &registry);
    ~GameObject();

    // TODO: read more about perfect forwarding
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

private:
    entt::entity entity;
    entt::registry &registry;
};