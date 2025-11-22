#pragma once

#include "core/SceneManager.h"
#include "core/Window.h"

#include "renderer/SpriteRenderer.h"

#include <string>

class BaseGame
{
public:
    virtual ~BaseGame() = default;

    virtual void onStart() = 0;
    virtual void onUpdate() = 0;
    virtual void onShutdown() = 0;

    // Entry point for the engine user (they call this, nothing else)
    void run()
    {
        onStart();
        runLoop();
        onShutdown();
    }

protected:
    void runLoop()
    {
        glfwSwapInterval(1);

        while (!glfwWindowShouldClose(m_window.getGlfwWindow()))
        {
            Time::update();
            glfwPollEvents();

            updateFrame();

            glfwSwapBuffers(m_window.getGlfwWindow());
        }

        glfwTerminate();
        m_running = false;
    }

    // when changing consider if we should make similar updates
    // to editor layer loop as well
    void updateFrame()
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // User logic
        onUpdate();

        // Engine logic
        m_sceneManager.update();

        // TODO: return const reference not shared_ptr. It is owned by the scene and is not shared.
        const Camera &cam = m_sceneManager.getActiveScene().getCamera();
        const auto &gameObjects = m_sceneManager.getActiveScene().getGameObjects();

        m_spriteRenderer.render(cam, gameObjects);
    }

public:
    // for now these should be publicly available
    Window m_window;
    SceneManager m_sceneManager;
    SpriteRenderer m_spriteRenderer;

    bool m_running = true;
};
