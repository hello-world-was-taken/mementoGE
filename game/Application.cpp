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
                sceneManager->getActiveGameObject().getComponent<Transform>().translate(500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::LeftArrow)
            {
                sceneManager->getActiveGameObject().getComponent<Transform>().translate(-500.0f * Time::deltaTime(), 0.0f, 0.0f);
            }
            else if (keyType == KeyType::DownArrow)
            {
                sceneManager->getActiveGameObject().getComponent<Transform>().translate(0.0f, -500.0f * Time::deltaTime(), 0.0f);
            }
            else if (keyType == KeyType::UpArrow)
            {
                sceneManager->getActiveGameObject().getComponent<Transform>().translate(0.0f, 500.0f * Time::deltaTime(), 0.0f);
            }
        }
    }
}

Application::Application()
    : mMouseListener{},
      // TODO: listener should be passed as a reference
      mWindow{mMouseListener, mEventHandler, 800, 600},
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
    mWindow.setupCallBack();
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