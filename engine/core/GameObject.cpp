#include "engine/core/GameObject.h"

GameObject::GameObject(entt::registry &registry, unsigned int width, unsigned int height) : m_registry(registry)
{
    m_entity = m_registry.create();
    m_width = width;
    m_height = height;
}

GameObject::~GameObject()
{
    std::cout << "Game Object destructor called" << std::endl;
    m_registry.destroy(m_entity);
}

void GameObject::destroy()
{
    m_registry.destroy(m_entity);
}

int &GameObject::getWidth()
{
    return m_width;
}

int &GameObject::getHeight()
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
