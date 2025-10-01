#pragma once

#include <memory>
#include <string>

#include "core/Scene.h"
#include "core/SceneManager.h"
#include "core/Camera.h"
#include "core/MouseActionController.h"

#include "opengl/FrameBuffer.h"

struct EditorContext
{

    Window &window;
    SceneManager sceneManager{&window};

    int viewportWidth = 1280;
    int viewportHeight = 720;

    bool drawGrid = false;
    std::string selectedTextureJsonPath;
    bool sceneImageHovered = false;

    // TODO: do we need activeScene? we can get it from scene manager
    std::shared_ptr<Scene> activeScene;
    // TODO: seems like only the gridRenderer uses this?
    std::shared_ptr<Camera> editorCamera{std::make_shared<Camera>(viewportWidth, viewportHeight)};

    // TODO: think more about the need for mouseActionController
    MouseActionController mouseActionController;
    FrameBuffer frameBuffer{viewportWidth, viewportHeight};
};
