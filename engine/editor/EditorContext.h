#pragma once

#include "core/Camera.h"
#include "core/FrameContext.h"
#include "core/MovementMode.h"
#include "core/Scene.h"
#include "core/Window.h"

#include "core/components/RenderLayer.h"
#include "core/components/Sprite.h"

#include "editor/Constants.h"
#include "editor/EditorInteractionMode.h"
#include "editor/EditorMouseController.h"
#include "editor/SceneHistory.h"

#include "renderer/Renderer2D.h"

#include <functional>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

// TODO: Consider moving this to a separate file, as it is not directly related to EditorContext along with the related
// functions below.
struct EditorWorkspaceState
{
    bool showScenePanel = true;
    bool showHierarchyPanel = true;
    bool showPropertiesPanel = true;
    bool showAssetsPanel = true;
    bool showSpriteSheetEditorPanel = true;
    bool showTileMapEditorPanel = true;
    bool showGameObjectEditorPanel = true;
    bool showPerformancePanel = true;
    bool showSystemsPanel = true;
    bool showLogPanel = true;
    bool showContextPanel = true;
    bool showMetricsWindow = false;
};

enum class RuntimeState
{
    Stopped,
    Playing,
    Paused,
};

struct EditorContext
{
    Window &window;
    // Set once before updating/drawing a frame; panels read the same captured time.
    FrameContext frame;

    float viewportWidth = WINDOW_WIDTH;
    float viewportHeight = WINDOW_HEIGHT;

    RuntimeState runtimeState = RuntimeState::Stopped;
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

    // Base interaction mode selected via the UI (toolbar).
    // Keyboard shortcuts can temporarily override interactionMode
    // but should not change baseInteractionMode.
    EditorInteractionMode interactionMode = EditorInteractionMode::None;
    EditorInteractionMode baseInteractionMode = EditorInteractionMode::None;

    MovementMode movementMode = MovementMode::Free;
    // We clear this when the active scene changes.
    std::vector<GameObject> selectedObjects;
    std::vector<glm::vec2> selectedGameObjectsDragOffset;

    // Serialized copies of selected objects for in-editor copy/paste.
    std::vector<std::string> clipboardSerializedObjects;

    std::unordered_map<std::string, Scene> sceneByScenePathMap;
    std::unordered_map<std::string, SceneHistory> sceneHistoryByScenePathMap;
    EditorCamera editorCamera;
    EditorMouseController editorMouseController;
    Renderer2D renderer2D;
    EditorWorkspaceState workspaceState;
    bool editorWorkspaceDirty = false;
    uint32_t editorRendererMask = static_cast<uint32_t>(RendererType::All);
    uint32_t runtimeRendererMask = static_cast<uint32_t>(RendererType::Sprite);

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
    bool isRuntimePlaying() const;
    bool isRuntimeActive() const;
    void snapshotScene();
    void startEdit();
    void endEdit();
    bool undoActiveScene();
    bool redoActiveScene();

    // TODO: move this to editor layer. EditorContext should be
    // just state as much as possible
    void startRuntimeScene();
    void resumeRuntimeScene();
    void pauseRuntimeScene();
    void stopRuntimeScene();

    void deserializeSelectedScene();
    void serializaActiveScene();
    void loadEditorWorkspaceState();
    void saveEditorWorkspaceState();
    void saveEditorWorkspaceStateIfDirty();
    void markEditorWorkspaceDirty();

    // Copy/paste selected GameObjects within the active scene.
    void copySelectedObjectsToClipboard();
    void pasteClipboardObjects();

    template <typename Func> void performSceneEdit(Func &&editFunc);
};

template <typename Func> void EditorContext::performSceneEdit(Func &&editFunc)
{
    Scene &scene = getActiveScene();
    getSelectedSceneHistory().pushSnapshot(scene); // before
    editFunc();
    getSelectedSceneHistory().pushSnapshot(scene); // after
}
