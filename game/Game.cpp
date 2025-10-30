#include "game/Game.h"
#include "game/systems/PatrolSystem.h"

#include "engine/core/Animator.h"
#include "engine/core/Event.h"
#include "engine/core/GameObject.h"
#include "engine/core/MouseListener.h"
#include "engine/core/Scene.h"
#include "engine/core/SceneManager.h"
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
        m_window.setupCallBack();
    }
    update();
}

void Game::processInput()
{
    auto &go = m_editorLayer.getEditorContext().sceneManager.getActiveScene().getPlayer();

    auto *eventHandler = EventHandler::instance();
    if (eventHandler->hasActiveEvent())
    {
        Event e = eventHandler->getCurrentEvent();

        if (e.getEventType() == EventType::Key || e.getEventType() == EventType::KeyRepeat)
        {
            KeyType keyType = e.getKeyType();

            if (keyType == KeyType::RightArrow)
            {
                go.getComponent<Transform>().position += glm::vec3{50.0f * Time::deltaTime(), 0.0f, 0.0f};
                go.getComponent<Sprite>().flipX = true;
                go.getComponent<Animator>().play("run");
            }
            else if (keyType == KeyType::LeftArrow)
            {
                go.getComponent<Transform>().position += glm::vec3{-50.0f * Time::deltaTime(), 0.0f, 0.0f};
                go.getComponent<Sprite>().flipX = false;
                go.getComponent<Animator>().play("run");
            }
        }
    }
    else
    {
        go.getComponent<Animator>().play("idle");
    }
}

void Game::update()
{
    // TODO: REMOVE
    bool static test = false;
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
                    if (!test)
                    {
                        test = true;
                        m_editorLayer.getEditorContext().sceneManager.getActiveScene().addSystem<PatrolSystem>();
                    }

                    processInput();
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
                processInput();
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