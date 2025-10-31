#pragma once

#include "core/Camera.h"
#include "core/Scene.h"
#include "core/SceneManager.h"

#include "editor/Constants.h"
#include "editor/EditorInteractionMode.h"
#include "editor/EditorMouseController.h"

#include "opengl/FrameBuffer.h"

#include <functional>
#include <memory>
#include <string>

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

    EditorInteractionMode interactionMode = EditorInteractionMode::Selection;
    std::vector<std::reference_wrapper<GameObject>> selectedObjects;

    ImVec2 propertiesPopupPos;
    ImVec2 createObjectPopupPos;
    glm::vec2 createObjectWorldPos;

    std::string selectedTextureJsonPath;

    EditorCamera editorCamera;
    EditorMouseController editorMouseController;
    FrameBuffer frameBuffer{viewportWidth, viewportHeight};
};
