#include <iostream>
#include <entt/entt.hpp>

#include "core/Scene.h"
#include "renderer/RenderBatch.h"
#include "core/MouseListener.h" // TODO: Game specific things shouldn't be here.

Scene::Scene()
{
    // NOTE: When trying to construct RenderBatch here, it was causing a segfault. I think it was because
    // the OpenGL context was not yet created which resulted in glGenVertexArrays == NULL. So, I moved the
    // RenderBatch construction to the start.
    // TODO: Investigate this further
    // m_renderBatch = new RenderBatch(this);
    m_gameObjects = std::make_shared<std::vector<std::shared_ptr<GameObject>>>();
    std::cout << "Scene constructor called" << std::endl;
}

Scene::~Scene()
{
    std::cout << "Scene destructor called" << std::endl;
    delete m_renderBatch;
}

void Scene::start()
{
    // TODO: can we pass the user character here and update that as the camera moves as well?
    // Camera moves right -> user moves right and environment moves left
    m_renderBatch = new RenderBatch(m_camera, m_gameObjects);
    m_renderBatch->render();
}

void Scene::update(float deltaTime, GLFWwindow *window)
{
    m_renderBatch->render();
    this->renderActiveGameObjectPropsImGui();
    MouseListener *listener = MouseListener::getListener();
    listener->getWorldCoordinates(m_camera);
}

std::shared_ptr<GameObject> Scene::addGameObject(unsigned int width, unsigned int height)
{
    std::shared_ptr<GameObject>
        gameObject = std::make_shared<GameObject>(m_registry, width, height);
    m_gameObjects->push_back(gameObject);

    std::cout << "Adding a scene with name ----- 3: " << std::endl;
    // Set the latest game object as the active game object
    m_activeGameObject = gameObject.get();

    return gameObject;
}

std::shared_ptr<std::vector<std::shared_ptr<GameObject>>> Scene::getGameObjects()
{
    return m_gameObjects;
}

// void Scene::addTextureToGameObject(std::shared_ptr<GameObject> gameObject, std::shared_ptr<Texture> texture)
// {
//     gameObject->addComponent<Sprite>(texture);
//     m_textures.push_back(texture);
// }

std::shared_ptr<Camera> Scene::getCamera()
{
    return m_camera;
}

void Scene::renderActiveGameObjectPropsImGui()
{
    if (m_activeGameObject == nullptr)
    {
        return;
    }
    ImGui::Begin("Properties");

    ImGui::Text("Size");
    int &width = m_activeGameObject->getWidth();
    int &height = m_activeGameObject->getHeight();
    ImGui::DragInt("Width", &width);
    ImGui::DragInt("Height", &height);
    ImGui::Separator();

    ImGui::Text("Transform");
    Transform &transform = m_activeGameObject->getComponent<Transform>();
    ImGui::DragFloat("x", &transform.getPosition()->x);
    ImGui::DragFloat("y", &transform.getPosition()->y);
    ImGui::DragFloat("z", &transform.getPosition()->z);
    ImGui::End();
}

//TODO: If no game object is present, it should throw
GameObject& Scene::getActiveGameObject()
{
    return *m_activeGameObject;
}

// TODO: what if the game object is not part of this scene?
bool Scene::setActiveGameObject(GameObject* gameObject)
{
    m_activeGameObject = gameObject;
    return true;
}