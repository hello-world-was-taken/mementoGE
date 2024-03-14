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

void addGameObject(std::shared_ptr<Scene> scene)
{
    // TODO: Think about creating an asset pool for textrues
    std::shared_ptr<Texture> texture = Resource::getTexture("slice01_01.png", false);
    texture->bind();

    std::shared_ptr<GameObject> gameObject = scene->addGameObject(32, 32);
    // TODO: We shouldn't be using glm::vec3 directly. We should have a class that wraps glm::vec3
    gameObject->addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f)); // every game object has a transform
    gameObject->addComponent<SpriteRenderer>(texture);

    std::shared_ptr<Texture> texture2 = Resource::getTexture("spritesheet_retina.png", true);
    texture2->bind();

    std::shared_ptr<GameObject> gameObject2 = scene->addGameObject(32, 16);
    gameObject2->addComponent<Transform>(glm::vec3(110.0f, 200.0f, 0.0f));
    gameObject2->addComponent<SpriteRenderer>(texture2, 256, 128, 0, 0);

    std::shared_ptr<GameObject> gameObject3 = scene->addGameObject(32, 16);
    gameObject3->addComponent<Transform>(glm::vec3(110.0f, 100.0f, 0.0f));
    gameObject3->addComponent<SpriteRenderer>(texture2, 128, 118, 2, 0);

    std::shared_ptr<GameObject> gameObject4 = scene->addGameObject(32, 16);
    gameObject4->addComponent<Transform>(glm::vec3(50.0f, 50.0f, 0.0f));
    gameObject4->addComponent<SpriteRenderer>(texture2, 128, 118, 2, 0);
}

// TODO: glfw_window should be abstracted away by core engine module
void eventHandler(GLFWwindow* glfw_window, SceneManager* sceneManager)
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

void run()
{
    // Create a window
    Window *window = Window::getWindow();
    window->initializeWindow();
    window->setupCallBack(
        MouseListener::cursorPositionCallback,
        MouseListener::mouseButtonCallback,
        MouseListener::scrollCallback,
        KeyListener::keyCallback);

    // Create a scene manager
    std::shared_ptr<SceneManager> scene_manager = std::make_shared<SceneManager>(window);

    // Create a scene. We need at least one scene to start the game
    std::shared_ptr<Scene> scene = std::make_shared<Scene>();
    scene_manager->setEventHandler(eventHandler);
    scene_manager->addScene("default_scene", scene);
    addGameObject(scene);

    // deserialize scene if we had any saved state
    scene_manager->deserialize();
    
    // Start the game loop
    scene_manager->gameLoop();
}