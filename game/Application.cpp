#include <memory>

#include "Application.h"
#include "engine/core/GameObject.h"
#include "engine/core/Window.h"
#include "engine/core/SceneManager.h"
#include "engine/core/Scene.h"
#include "engine/core/Resource.h"
#include "engine/core/Sprite.h"
#include "engine/core/EventHandler.h"
#include "engine/core/Event.h"

void eventHandler2(Window &window, SceneManager *sceneManager, const EventHandler &eventHandler)
{
    if (eventHandler.hasActiveEvent())
    {
        Event e = eventHandler.getCurrentEvent();

        if (e.getEventType() == EventType::Key)
        {
            KeyType keyType = e.getKeyType();

            if (keyType == KeyType::Escape)
            {
                window.closeWindow();
                std::cout << "Escape" << std::endl;
            }
            else if (keyType == KeyType::RightArrow)
            {
                // sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(-500.0f * Time::deltaTime(), 0.0f, 0.0f));
                sceneManager->getActiveGameObject().getComponent<Transform>().translate(-500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::LeftArrow)
            {
                // sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(500.0f * Time::deltaTime(), 0.0f, 0.0f));
                sceneManager->getActiveGameObject().getComponent<Transform>().translate(500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::DownArrow)
            {
                // sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(0.0f, 500.0f * Time::deltaTime(), 0.0f));
                sceneManager->getActiveGameObject().getComponent<Transform>().translate(0.0f, 500.0f * Time::deltaTime(), 0.0f);
            }
            else if (keyType == KeyType::UpArrow)
            {
                // sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(0.0f, -500.0f * Time::deltaTime(), 0.0f));
                sceneManager->getActiveGameObject().getComponent<Transform>().translate(0.0f, -500.0f * Time::deltaTime(), 0.0f);
            }
        }
    }
}

Application::Application()
    : mMouseListener{},
      mKeyListener{},
      // TODO: listener should be passed as a reference
      mWindow{mMouseListener, mKeyListener, 800, 600},
      mSceneManager{&mWindow, mEventHandler}
{
}

Application::~Application()
{
}

void Application::setup()
{
    mSceneManager.deserialize();
    mSceneManager.setEventHandler(eventHandler2);
    // mSceneManager.addScene("default_scene", Scene{"default_scene_original"});
    // mSceneManager.start();

    // // GAME OBJECT 1 Tile: Player character
    // mSceneManager.getActiveScene()->addGameObject(32, 32, "tile");
    // GameObject &activeGameObject = mSceneManager.getActiveGameObject();
    // mPlayerCharacter = &activeGameObject;

    // // TODO: We shouldn't be using glm::vec3 directly. We should have a class that wraps glm::vec3
    // // Grass Tile
    // activeGameObject.addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f));
    // activeGameObject.addComponent<Sprite>("../assets/texture/slice01_01.png", false);

    // // GAME OBJECT 2 Cloud
    // mSceneManager.getActiveScene()->addGameObject(32, 16, "cloud");
    // GameObject &could = mSceneManager.getActiveGameObject();

    // could.addComponent<Transform>(glm::vec3(110.0f, 200.0f, 0.0f));
    // could.addComponent<Sprite>("../assets/texture/spritesheet_retina.png", true, 2, 1, 128, 0, 10);

    // TODO: MAKE SURE ACTIVE GAME OBJECT IS PLAYER WHEN DESERIALIZING
    // deserialize scene if we had any saved state
    // mSceneManager.deserialize();
}

void Application::start()
{
    setup();
    processInput();
    update();
    render();

    // Start the game loop
    mSceneManager.gameLoop();
}

void Application::processInput()
{
    mWindow.setupCallBack(mEventHandler);
}

void Application::update()
{
}

void Application::render()
{
}

void Application::destroy()
{
}