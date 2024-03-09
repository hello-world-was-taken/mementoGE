#include <memory>

#include "Application.h"
#include "engine/opengl/Texture.h"  // TODO: anything out side of core should not be included here
#include "engine/core/GameObject.h"
#include "engine/core/Window.h"
#include "engine/core/SceneManager.h"
#include "engine/core/Scene.h"

void addGameObject(std::shared_ptr<Scene> scene)
{
    // TODO: Think about creating an asset pool for textrues
    const char *texture_path = "../assets/texture/slice01_01.png";
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(texture_path, 0);
    texture->bind();

    std::shared_ptr<GameObject> gameObject = scene->addGameObject(70, 70);
    // TODO: We shouldn't be using glm::vec3 directly. We should have a class that wraps glm::vec3
    gameObject->addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f)); // every game object has a transform
    gameObject->addComponent<SpriteRenderer>(texture);

    const char *texture_path2 = "../assets/texture/spritesheet_retina.png";
    std::shared_ptr<Texture> texture2 = std::make_shared<Texture>(texture_path2, 1, true);
    // std::shared_ptr<Texture> texture2 = std::make_shared<Texture>(texture_path2, 1);
    texture2->bind();

    std::shared_ptr<GameObject> gameObject2 = scene->addGameObject(128, 64);
    gameObject2->addComponent<Transform>(glm::vec3(110.0f, 200.0f, 0.0f)); // every game object has a transform
    gameObject2->addComponent<SpriteRenderer>(texture2, 256, 128, 0, 0);
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