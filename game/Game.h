#pragma once

#include "engine/core/SceneManager.h"
#include "engine/core/Window.h"

#include "engine/editor/EditorLayer.h"

#include "engine/renderer/SpriteRenderer.h"

class Game
{
public:
    Game(bool editorMode = false);
    ~Game();

    /**
     * @brief start is the entry point of our app/game
     * will process userinput, update our state and render.
     */
    void start();
    void processInput();
    void update();
    void render();
    void destroy();

private:
    Window m_window;
    EditorLayer m_editorLayer;
    SceneManager m_sceneManager;
    SpriteRenderer m_spriteRenderer;
    // TODO: what is the advantage of having these here?
    // Even after being extracted to Event Handling system, do we need it here?
    GameObject *mPlayerCharacter = nullptr;

    bool m_editorMode;
};