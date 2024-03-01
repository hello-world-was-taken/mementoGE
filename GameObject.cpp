#include "GameObject.h"


class GameObject
{
public:
    GameObject(entt::registry &registry) : registry(registry)
    {
        entity = registry.create();
    }

    ~GameObject()
    {
        registry.destroy(entity);
    }

    // TODO: read more about perfect forwarding
    template <typename Component, typename... Args>
    void addComponent(Args &&...args)
    {
        registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template <typename Component>
    bool hasComponent() const
    {
        return registry.has<Component>(entity);
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

    void destroy()
    {
        registry.destroy(entity);
    }

private:
    entt::entity entity;
    entt::registry &registry;
};