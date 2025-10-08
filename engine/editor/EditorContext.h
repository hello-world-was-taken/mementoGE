#pragma once

#include "core/Scene.h"
#include "core/SceneManager.h"
#include "core/Camera.h"

#include "editor/EditorMouseController.h"
#include "editor/Constants.h"

#include <memory>
#include <string>

#include "opengl/FrameBuffer.h"

struct EditorContext
{

    Window &window;
    SceneManager sceneManager{&window};

    float viewportWidth = WINDOW_WIDTH;
    float viewportHeight = WINDOW_HEIGHT;

    bool drawGrid = false;
    bool sceneImageHovered = false;

    bool showPropertiesPopup = false;
    bool showCreateObjectPopup = false;

    ImVec2 propertiesPopupPos;
    ImVec2 createObjectPopupPos;
    glm::vec2 createObjectWorldPos;

    std::string selectedTextureJsonPath;

    EditorCamera editorCamera;
    EditorMouseController editorMouseController;
    FrameBuffer frameBuffer{viewportWidth, viewportHeight};
};
