#include "engine/Window.h"
#include "engine/SceneManager.h"
#include "engine/Scene.h"
#include <memory>
#include "Application.h"
#include "engine/Texture.h"
#include "engine/GameObject.h"

void addGameObject(std::shared_ptr<Scene> scene)
{
    // TODO: Think about creating an asset pool for textrues
    const char *texture_path = "../assets/texture/slice01_01.png";
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(texture_path, 0.0f);
    texture->bind();

    std::shared_ptr<GameObject> gameObject = scene->addGameObject();
    gameObject->addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f)); // every game object has a transform
    gameObject->addComponent<SpriteRenderer>(texture);

    const char *texture_path2 = "../assets/texture/sheet.png";
    std::shared_ptr<Texture> texture2 = std::make_shared<Texture>(texture_path2, 1);
    texture2->bind();

    std::shared_ptr<GameObject> gameObject2 = scene->addGameObject();
    gameObject2->addComponent<Transform>(glm::vec3(110.0f, 0.0f, 0.0f)); // every game object has a transform
    gameObject2->addComponent<SpriteRenderer>(texture2);
}

void run() {
    // Create a window
    Window *window = Window::getWindow();
    window->initializeWindow();

    // Create a scene manager
    // TODO: whoever is creating a game shouldn't have to worry about glfwWindow
    //       needs to be designed better.
    std::shared_ptr<SceneManager> scene_manager = std::make_shared<SceneManager>(window->getGlfwWindow());

    // Add the scene manager to the window. This should be called before run
    window->addSceneManager(scene_manager);
    
    // Create a scene. We need at least one scene to start the game
    std::shared_ptr<Scene> scene = std::make_shared<Scene>();
    scene_manager->addScene("triangle_scene",scene);
    addGameObject(scene);
    // Start the main loop
    window->run();
}