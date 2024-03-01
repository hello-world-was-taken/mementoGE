#include "GameObject.h"

GameObject::GameObject(entt::registry &registry) : registry(registry)
{
    entity = registry.create();
}

GameObject::~GameObject()
{
    registry.destroy(entity);
}

void GameObject::destroy()
{
    registry.destroy(entity);
}
