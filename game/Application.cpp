#include <memory>
#include <iostream>

#include "Application.h"
#include "engine/core/GameObject.h"
#include "engine/core/Window.h"
#include "engine/core/MouseListener.h"
#include "engine/core/SceneManager.h"
#include "engine/core/Scene.h"
#include "engine/core/Sprite.h"
#include "engine/core/Event.h"

#include "util/Time.h"

Application::Application(bool editorMode)
    : // using 16:9 for window size to match our virtual screen setup
      mWindow{1280, 720},
      mSceneManager{&mWindow},
      m_editorMode{editorMode},
      m_editorLayer{mWindow}
{
}

Application::~Application()
{
}

void Application::setup()
{
    if (m_editorMode)
    {
        m_editorLayer.prepare();
    }
    else
    {
        mSceneManager.deserialize();
        mSceneManager.prepare();
    }
}

void Application::start()
{
    setup();
    processInput();
    update();
}

void Application::processInput()
{
    if (m_editorMode)
    {
        mWindow.setupCallBack();
    }
}

void Application::update()
{
    mWindow.run(
        [&]()
        {
            if (m_editorMode)
            {
                m_editorLayer.update();
                MouseListener::get()->beginFrame();
            }
            else
            {
                mSceneManager.update();
            }
        },
        [&]()
        {
            destroy();
        });
}

void Application::destroy()
{
}