#include "Scene.h"
#include "engine/RenderBatch.h"
#include <iostream>

Scene::Scene()
{
    // NOTE: When trying to construct RenderBatch here, it was causing a segfault. I think it was because
    // the OpenGL context was not yet created which resulted in glGenVertexArrays == NULL. So, I moved the
    // RenderBatch construction to the start.
    // TODO: Investigate this further
    // m_renderBatch = new RenderBatch(this);
}

Scene::~Scene()
{
    std::cout << "Scene destructor called" << std::endl;
    delete m_renderBatch;
    for (auto texture : m_textures)
    {
        delete texture;
    }
}

void Scene::start(GLFWwindow *window)
{
    m_renderBatch = new RenderBatch(&m_camera, &gameObjects);
    m_renderBatch->render();
}

void Scene::update(float deltaTime, GLFWwindow *window)
{
    m_renderBatch->render();
}

GameObject *Scene::addGameObject()
{
    // TODO: Think about creating an asset pool for textrues
    const char *texture_path = "../assets/texture/slice01_01.png";
    Texture *texture = new Texture(texture_path, 0.0f);
    m_textures.push_back(texture);
    texture->bind();

    GameObject *gameObject = new GameObject(m_registry);
    gameObjects.push_back(*gameObject);
    gameObject->addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f)); // every game object has a transform

    gameObject->addComponent<SpriteRenderer>(texture);

    const char *texture_path2 = "../assets/texture/sheet.png";
    Texture *texture2 = new Texture(texture_path2, 1);
    m_textures.push_back(texture2);
    texture2->bind();

    GameObject *gameObject2 = new GameObject(m_registry);
    gameObjects.push_back(*gameObject2);
    gameObject2->addComponent<Transform>(glm::vec3(110.0f, 0.0f, 0.0f)); // every game object has a transform

    gameObject2->addComponent<SpriteRenderer>(texture2);
    return nullptr;
}

Camera *Scene::getCamera()
{
    return &m_camera;
}