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

    Window m_window;
    SceneManager m_sceneManager;
    SpriteRenderer m_spriteRenderer;
};
