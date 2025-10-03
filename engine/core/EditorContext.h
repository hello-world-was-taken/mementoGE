#pragma once

#include <memory>
#include <string>

#include "core/Scene.h"
#include "core/SceneManager.h"
#include "core/Camera.h"
#include "core/EditorMouseController.h"

#include "opengl/FrameBuffer.h"

struct EditorContext
{

    Window &window;
    SceneManager sceneManager{&window};

    float viewportWidth = 1280.0f;
    float viewportHeight = 720.0f;

    bool drawGrid = false;
    std::string selectedTextureJsonPath;
    bool sceneImageHovered = false;
    bool showPropertiesPopup = false;
    ImVec2 propertiesPopupPos;

    // TODO: do we need activeScene? we can get it from scene manager
    std::shared_ptr<Scene> activeScene;
    // TODO: seems like only the gridRenderer uses this?
    std::shared_ptr<Camera> editorCamera{std::make_shared<Camera>(viewportWidth, viewportHeight)};

    EditorMouseController editorMouseController;
    FrameBuffer frameBuffer{viewportWidth, viewportHeight};
};
