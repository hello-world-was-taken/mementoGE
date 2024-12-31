#include <memory>

#include "Application.h"
#include "engine/opengl/Texture.h" // TODO: anything out side of core should not be included here
#include "engine/core/GameObject.h"
#include "engine/core/Window.h"
#include "engine/core/SceneManager.h"
#include "engine/core/Scene.h"
#include "game/MouseListener.h" // TODO: this should be abstracted away by core engine module
#include "game/KeyListener.h"   // TODO: this should be abstracted away by core engine module
#include "engine/core/Resource.h"
#include "engine/core/Sprite.h"

void addGameObject(Scene* scene)
{
    // TODO: Think about creating an asset pool for textures
    std::shared_ptr<Texture> texture = Resource::getTexture("../assets/texture/slice01_01.png", false);
    texture->bind();
    std::cout << "---------------------";
    std::shared_ptr<GameObject> gameObject = scene->addGameObject(32, 32);
    std::cout << "Adding a scene with name: " << std::endl;
    // TODO: We shouldn't be using glm::vec3 directly. We should have a class that wraps glm::vec3
    // Grass Tile
    gameObject->addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f));
    gameObject->addComponent<Sprite>(texture);

    std::shared_ptr<Texture> texture2 = Resource::getTexture("../assets/texture/spritesheet_retina.png", true);
    texture2->bind();

    // Cloud
    std::shared_ptr<GameObject> gameObject2 = scene->addGameObject(32, 16);
    gameObject2->addComponent<Transform>(glm::vec3(110.0f, 200.0f, 0.0f));
    gameObject2->addComponent<Sprite>(texture2, 2, 1, 128, 0, 10);

    // Dialog box
    std::shared_ptr<GameObject> gameObject3 = scene->addGameObject(32, 16);
    gameObject3->addComponent<Transform>(glm::vec3(110.0f, 100.0f, 0.0f));
    gameObject3->addComponent<Sprite>(texture2, 1, 1, 128, 2, 10);

    // Dialog box
    std::shared_ptr<GameObject> gameObject4 = scene->addGameObject(32, 16);
    gameObject4->addComponent<Transform>(glm::vec3(50.0f, 50.0f, 0.0f));
    gameObject4->addComponent<Sprite>(texture2, 1, 1, 128, 2, 10);

    // Number 4
    std::cout << "Number 4" << std::endl;
    std::shared_ptr<GameObject> gameObject5 = scene->addGameObject(32, 16);
    gameObject5->addComponent<Transform>(glm::vec3(110.0f, 130.0f, 0.0f));
    gameObject5->addComponent<Sprite>(texture2, 1, 1, 128, 0, 0);
}

// TODO: glfw_window should be abstracted away by core engine module
void eventHandler(GLFWwindow *glfw_window, SceneManager *sceneManager)
{
    if (KeyListener::isKeyPressed(GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(glfw_window, true);
        std::cout << "Escape" << std::endl;
    }
    else if (KeyListener::isKeyPressed(GLFW_KEY_RIGHT))
    {
        sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(-500.0f * Time::deltaTime(), 0.0f, 0.0f));
    }
    else if (KeyListener::isKeyPressed(GLFW_KEY_LEFT))
    {
        sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(500.0f * Time::deltaTime(), 0.0f, 0.0f));
    }
    else if (KeyListener::isKeyPressed(GLFW_KEY_DOWN))
    {
        sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(0.0f, 500.0f * Time::deltaTime(), 0.0f));
    }
    else if (KeyListener::isKeyPressed(GLFW_KEY_UP))
    {
        sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(0.0f, -500.0f * Time::deltaTime(), 0.0f));
    }
}

Application::Application()
    : m_window{800, 600},
      m_scene_manager{&m_window}
{
}

Application::~Application()
{
}

void Application::setup()
{
}

void Application::run()
{
    setup();
    update();
    render();

    m_window.setupCallBack(
        MouseListener::cursorPositionCallback,
        MouseListener::mouseButtonCallback,
        MouseListener::scrollCallback,
        KeyListener::keyCallback);

    // Create a scene. We need at least one scene to start the game
    Scene scene;
    m_scene_manager.setEventHandler(eventHandler);
    m_scene_manager.addScene("default_scene", scene);
    m_scene_manager.start();
    addGameObject(m_scene_manager.getActiveScene());

    // deserialize scene if we had any saved state
    m_scene_manager.deserialize();

    // Start the game loop
    m_scene_manager.gameLoop();
}

void Application::update()
{
}

void Application::render()
{
}