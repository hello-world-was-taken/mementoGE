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
#include "engine/core/Constants.h"

#include "util/Time.h"

Application::Application(bool editorMode)
    : m_window{},
      m_sceneManager{&m_window},
      m_editorMode{editorMode},
      m_editorLayer{m_window}
{
}

Application::~Application()
{
}

void Application::start()
{
    processInput();
    update();
}

void Application::processInput()
{
    if (m_editorMode)
    {
        m_window.setupCallBack();
    }
}

void Application::update()
{
    m_window.run(
        [&]()
        {
            // TODO: can we make this compile time branching and would that help us remove
            // all the editor specific code from our game?
            if (m_editorMode)
            {
                m_editorLayer.update();
                MouseListener::instance()->beginFrame();
            }
            else
            {
                // TODO: do we need to bind default buffer
                // (initially added since we used a custom buffer in the editor)
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                m_sceneManager.update();
                render();
            }
        },
        [&]()
        {
            destroy();
        });
}

void Application::render()
{
    // TODO: return const reference not shared_ptr. It is owned by the scene and is not shared.
    const Camera &cam = m_sceneManager.getActiveScene().getCamera();
    const std::vector<GameObject> &gameObjects = m_sceneManager.getActiveScene().getGameObjects();

    m_spriteRenderer.render(cam, gameObjects);
}

void Application::destroy()
{
}