#include "GameObject.h"

GameObject::GameObject(entt::registry &registry) : registry(registry)
{
    entity = registry.create();
}

GameObject::~GameObject()
{
    std::cout << "Game Object destructor called" << std::endl;
    registry.destroy(entity);
}

void GameObject::destroy()
{
    registry.destroy(entity);
}
