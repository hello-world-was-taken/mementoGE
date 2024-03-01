#include <entt/entt.hpp>


class GameObject
{
public:
    GameObject(entt::registry &registry);
    ~GameObject();

    template <typename Component, typename... Args>
    void addComponent(Args &&...args);

    template <typename Component>
    bool hasComponent() const;

    template <typename Component>
    Component &getComponent();

    template <typename Component>
    void removeComponent();

    void destroy();

private:
    entt::entity entity;
    entt::registry &registry;
};