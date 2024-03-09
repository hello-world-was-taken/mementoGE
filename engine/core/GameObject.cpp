#include "engine/core/GameObject.h"

GameObject::GameObject(entt::registry &registry, unsigned int width, unsigned int height) : registry(registry)
{
    entity = registry.create();
    m_width = width;
    m_height = height;
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

unsigned int GameObject::getWidth()
{
    return m_width;
}

unsigned int GameObject::getHeight()
{
    return m_height;
}

std::vector<glm::vec3> GameObject::getQuad()
{
    return std::vector<glm::vec3>({{0.0f, 1.0f * m_height, 0.0f},             // top left
                                   {0.0f, 0.0f, 0.0f},                        // bottom left
                                   {1.0f * m_width, 0.0f, 0.0f},              // bottom right
                                   {1.0f * m_width, 1.0f * m_height, 0.0f}}); // top right
}
