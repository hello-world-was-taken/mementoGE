#include "game/Game.h"
#include "game/systems/PatrolSystem.h"
#include "game/systems/PlayerControllerSystem.h"

#include "engine/core/Animator.h"
#include "engine/core/Event.h"
#include "engine/core/GameObject.h"
#include "engine/core/MouseListener.h"
#include "engine/core/Scene.h"
#include "engine/core/SceneManager.h"
#include "engine/core/SystemRegistry.h"
#include "engine/core/Window.h"
#include "engine/core/components/Sprite.h"

#include "engine/editor/Constants.h"

#include "util/Time.h"

#include <iostream>
#include <memory>

Game::Game(bool editorMode) : m_window{}, m_sceneManager{&m_window}, m_editorMode{editorMode}, m_editorLayer{m_window}
{
}

Game::~Game()
{
}

void Game::start()
{
    if (m_editorMode)
    {
        // register systems
        SystemRegistry::instance().registerSystem<PatrolSystem>("PatrolSystem");
        SystemRegistry::instance().registerSystem<PlayerControllerSystem>("PlayerControllerSystem");

        m_editorLayer.getEditorContext().sceneManager.deserialize();
        m_editorLayer.getEditorContext().sceneManager.getActiveScene().addSystem("PatrolSystem");

        m_window.setupCallBack();
    }
    update();
}

void Game::update()
{
    m_window.run(
        [&]()
        {
            // TODO: can we make this compile time branching and would that help us remove
            // all the editor specific code from our game?
            if (m_editorMode)
            {
                m_editorLayer.update();
                if (m_editorLayer.getEditorContext().sceneManager.isPlaying())
                {
                    m_editorLayer.getEditorContext().sceneManager.getActiveScene().addSystem("PlayerControllerSystem");
                }
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
                // TODO: we need to set the player as active game object in game mode
                m_editorLayer.getEditorContext().sceneManager.getActiveScene().addSystem("PlayerControllerSystem");
                render();
            }
        },
        [&]()
        {
            destroy();
        });
}

void Game::render()
{
    // TODO: return const reference not shared_ptr. It is owned by the scene and is not shared.
    const Camera &cam = m_sceneManager.getActiveScene().getCamera();
    const std::vector<GameObject> &gameObjects = m_sceneManager.getActiveScene().getGameObjects();

    m_spriteRenderer.render(cam, gameObjects);
}

void Game::destroy()
{
}