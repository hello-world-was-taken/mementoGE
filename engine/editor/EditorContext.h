#pragma once

#include "core/Camera.h"
#include "core/Scene.h"
#include "core/SceneManager.h"
#include "core/Window.h"

#include "core/components/RenderLayer.h"
#include "core/components/Sprite.h"

#include "editor/Constants.h"
#include "editor/EditorInteractionMode.h"
#include "editor/EditorMouseController.h"
#include "editor/SceneHistory.h"

#include "renderer/Renderer2D.h"

#include <functional>
#include <map>
#include <memory>
#include <string>

struct EditorContext
{
    Window &window;

    float viewportWidth = WINDOW_WIDTH;
    float viewportHeight = WINDOW_HEIGHT;

    bool isPlaying = false;
    bool editingInProgress = false;

    bool drawGrid = false;
    bool sceneImageHovered = false;

    bool showPropertiesPopup = false;
    ImVec2 propertiesPopupPos;

    bool showCreateObjectPopup = false;
    ImVec2 createObjectPopupPos;
    glm::vec2 createObjectWorldPos;

    // scene preview points
    ImVec2 scenePanelTopLeftPos;
    ImVec2 scenePanelSize;

    // TODO: improve the namings
    std::string selectedScenePath;
    std::string selectedTextureJsonPath;
    std::string selectedAssetChildFolderPath;
    std::string selectedFontPath;

    EditorInteractionMode interactionMode = EditorInteractionMode::None;
    std::vector<std::reference_wrapper<GameObject>> selectedObjects;
    std::vector<glm::vec2> selectedGameObjectsDragOffset;

    std::unordered_map<std::string, Scene> sceneByScenePathMap;
    std::unordered_map<std::string, SceneHistory> sceneHistoryByScenePathMap;
    EditorCamera editorCamera;
    EditorMouseController editorMouseController;
    Renderer2D renderer2D;

    std::unordered_map<RenderLayerType, bool> renderLayerVisibility = {
        {RenderLayerType::Background, true},
        {RenderLayerType::World, true},
        {RenderLayerType::Foreground, true},
        {RenderLayerType::UI, true},
        {RenderLayerType::Debug, true},
    };

    /*
     * screen is glfw window
     * local is scene preview imgui window
     * framebuffer is, well, the framebuffer
     * world is game world
     */
    glm::vec2 screenToLocal(glm::vec2 imageSize);
    glm::vec2 localToFrameBuffer(glm::vec2 localPos);
    glm::vec2 frameBufferToWorld(glm::vec2 framebufferPos);
    glm::vec2 getWorldCoordinate(glm::vec2 mouseScreenCoords);

    // get the screen coordinates (glfw screen) from world coordinate (game world)
    glm::vec2 getScreenCoordinate(glm::vec2 worldPos);
    glm::vec2 worldToFrameBuffer(glm::vec2 worldPos);
    glm::vec2 frameBufferToLocal(glm::vec2 frameBufferPos);
    glm::vec2 localToScreen(glm::vec2 localPos);

    SceneHistory &getSelectedSceneHistory();

    Scene &getActiveScene();
    void snapshotScene();
    void startEdit();
    void endEdit();

    // TODO: move this to editor layer. EditorContext should be
    // just state as much as possible
    void startRuntimeScene();
    void pauseRuntimeScene();
    void stopRuntimeScene();

    void deserializeSelectedScene();
    void serializaActiveScene();

    template <typename Func> void performSceneEdit(Func &&editFunc);
};

template <typename Func> void EditorContext::performSceneEdit(Func &&editFunc)
{
    Scene &scene = getActiveScene();
    getSelectedSceneHistory().pushSnapshot(scene); // before
    editFunc();
    getSelectedSceneHistory().pushSnapshot(scene); // after
}
