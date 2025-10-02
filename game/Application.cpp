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
                MouseListener::instance()->beginFrame();
            }
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

                glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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