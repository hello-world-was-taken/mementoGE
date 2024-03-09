#include <iostream>

#include "engine/core/Scene.h"
#include "engine/renderer/RenderBatch.h"

Scene::Scene()
{
    // NOTE: When trying to construct RenderBatch here, it was causing a segfault. I think it was because
    // the OpenGL context was not yet created which resulted in glGenVertexArrays == NULL. So, I moved the
    // RenderBatch construction to the start.
    // TODO: Investigate this further
    // m_renderBatch = new RenderBatch(this);
    m_gameObjects =  std::make_shared<std::vector<std::shared_ptr<GameObject>>>();
}

Scene::~Scene()
{
    std::cout << "Scene destructor called" << std::endl;
    delete m_renderBatch;
}

void Scene::start(GLFWwindow *window)
{
    m_renderBatch = new RenderBatch(&m_camera, m_gameObjects);
    m_renderBatch->render();
}

void Scene::update(float deltaTime, GLFWwindow *window)
{
    m_renderBatch->render();
}

std::shared_ptr<GameObject> Scene::addGameObject(unsigned int width, unsigned int height)
{
    std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(m_registry, width, height);
    m_gameObjects->push_back(gameObject);

    return gameObject;
}

// void Scene::addTextureToGameObject(std::shared_ptr<GameObject> gameObject, std::shared_ptr<Texture> texture)
// {
//     gameObject->addComponent<SpriteRenderer>(texture);
//     m_textures.push_back(texture);
// }

Camera *Scene::getCamera()
{
    return &m_camera;
}