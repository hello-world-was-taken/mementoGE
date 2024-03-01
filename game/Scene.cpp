#include "Scene.h"
#include "engine/RenderBatch.h"
#include <iostream>

Scene::Scene()
{
    // TODO: why was this causing segfault while trying to call glGenVertexArrays which turned
    // out to be NULL. i.e glGenVertexArrays == NULL?
    // m_renderBatch = new RenderBatch(this);
}

Scene::~Scene()
{
    std::cout << "Scene destructor called" << std::endl;
    delete m_renderBatch;
}

void Scene::start(GLFWwindow *window)
{
    m_renderBatch = new RenderBatch(this);
    m_renderBatch->render();
}

void Scene::update(float deltaTime, GLFWwindow *window)
{
    m_renderBatch->render();
}

GameObject *Scene::addGameObject()
{
    GameObject *gameObject = new GameObject(m_registry);
    gameObjects.push_back(*gameObject);
    gameObject->addComponent<Transform>(); // every game object has a transform
    return gameObject;
}

Camera *Scene::getCamera()
{
    return &m_camera;
}