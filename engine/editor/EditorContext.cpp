#include "editor/EditorContext.h"

#include "core/ComponentRegistry.h"
#include "core/components/Animator.h"
#include "core/components/AudioSource.h"
#include "core/components/EntityInfo.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/Patrol.h"
#include "core/components/PostProcessSettings.h"
#include "core/components/RenderLayer.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"
#include "core/components/TextAnchor.h"
#include "core/components/Transform.h"

#include "util/PathUtils.h"
#include "util/LogChannels.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sawyer/Logger.h>
#include <yaml-cpp/yaml.h>

glm::vec2 EditorContext::getWorldCoordinate(glm::vec2 mouseScreenCoords)
{
    glm::vec2 localPos = screenToLocal(mouseScreenCoords);
    glm::vec2 framebufferPos = localToFrameBuffer(localPos);

    // Make sure the mouse is inside the image
    if (localPos.x < 0 || localPos.y < 0 || localPos.x > scenePanelSize.x || localPos.y > scenePanelSize.y)
    {
        return glm::vec2(-1.0f);
    }

    return frameBufferToWorld(framebufferPos);
}

glm::vec2 EditorContext::screenToLocal(glm::vec2 mousePos)
{
    // Get position of mouse relative to imgui scene preview window
    float localX = mousePos.x - scenePanelTopLeftPos.x;
    float localY = mousePos.y - scenePanelTopLeftPos.y;

    // Flip Y because ImGui has origin at top-left, OpenGL at bottom-left
    localY = scenePanelSize.y - localY;

    return {localX, localY};
}

glm::vec2 EditorContext::localToFrameBuffer(glm::vec2 localPos)
{
    float fbX = (localPos.x / scenePanelSize.x) * renderer2D.m_finalFBO.getWidth();
    float fbY = (localPos.y / scenePanelSize.y) * renderer2D.m_finalFBO.getHeight();

    return {fbX, fbY};
}

glm::vec2 EditorContext::frameBufferToWorld(glm::vec2 fbPos)
{
    // Convert to Normalized Device Coordinates (NDC)
    float ndcX = (fbPos.x / renderer2D.m_finalFBO.getWidth()) * 2.0f - 1.0f;
    float ndcY = (fbPos.y / renderer2D.m_finalFBO.getHeight()) * 2.0f - 1.0f;

    // TODO: do we need to update z to layer images over one another?
    glm::vec4 clipCoords = glm::vec4(ndcX, ndcY, 0.0f, 1.0f);

    glm::mat4 viewProj = editorCamera.getProjectionMatrix() * editorCamera.getViewMatrix();
    glm::mat4 invViewProj = glm::inverse(viewProj);

    glm::vec4 worldCoords = invViewProj * clipCoords;

    return glm::vec2(worldCoords.x, worldCoords.y);
}

glm::vec2 EditorContext::getScreenCoordinate(glm::vec2 worldPos)
{
    glm::vec2 frameBufferPos = worldToFrameBuffer(worldPos);
    glm::vec2 localPos = frameBufferToLocal(frameBufferPos);

    return localToScreen(localPos);
}

glm::vec2 EditorContext::worldToFrameBuffer(glm::vec2 worldPos)
{
    glm::mat4 viewProj = editorCamera.getProjectionMatrix() * editorCamera.getViewMatrix();

    // transform world position to clip space
    glm::vec4 clipSpaceCoords = viewProj * glm::vec4(worldPos, 0.0f, 1.0f);

    // check if w is zero (could be for certain cases like points at infinity)
    if (clipSpaceCoords.w == 0.0f)
    {
        LOG_WARN(LogChannel::Editor, "Invalid world-to-framebuffer transformation: w is zero");
        return glm::vec2(0.0f, 0.0f);
    }

    // convert clip space to normalized device coordinates (NDC)
    glm::vec3 ndcCoords = clipSpaceCoords / clipSpaceCoords.w;

    // map NDC to screen space (viewport coordinates)
    glm::vec2 framebufferCoords;
    framebufferCoords.x =
        (ndcCoords.x + 1.0f) * 0.5f * viewportWidth; // TODO: shouldn't we use framebuffer width/height here?
    framebufferCoords.y = (1.0f - ndcCoords.y) * 0.5f * viewportHeight; // flip Y-axis for imgui

    // LOG_DEBUG(LogChannel::Editor, "World position: ({}, {})", worldPos.x, worldPos.y);
    // LOG_DEBUG(LogChannel::Editor, "Framebuffer coordinates: ({}, {})", framebufferCoords.x, framebufferCoords.y);

    return framebufferCoords;
}

glm::vec2 EditorContext::frameBufferToLocal(glm::vec2 frameBufferPos)
{
    float localPosX = (frameBufferPos.x / viewportWidth) * scenePanelSize.x;
    float localPosY = (frameBufferPos.y / viewportHeight) * scenePanelSize.y;

    return {localPosX, localPosY};
}

glm::vec2 EditorContext::localToScreen(glm::vec2 localPos)
{
    glm::vec2 screenPos = localPos + glm::vec2{scenePanelTopLeftPos.x, scenePanelTopLeftPos.y};

    return screenPos;
}

SceneHistory &EditorContext::getSelectedSceneHistory()
{
    return sceneHistoryByScenePathMap[selectedScenePath];
}

Scene &EditorContext::getActiveScene()
{
    if (isRuntimeActive())
    {
        auto it = sceneByScenePathMap.find("runtime_scene");
        if (it != sceneByScenePathMap.end())
        {
            return it->second;
        }
        else
        {
            std::cerr << "Runtime is active but runtime_scene was not found" << std::endl;
            runtimeState = RuntimeState::Stopped;
        }
    }

    if (!selectedScenePath.empty())
    {
        auto it = sceneByScenePathMap.find(selectedScenePath);
        if (it == sceneByScenePathMap.end())
        {
            // deserilizes and adds selected scene to our map
            deserializeSelectedScene();
            getSelectedSceneHistory().pushInitialScene(getActiveScene()); // push initial saved state
        }
        return sceneByScenePathMap.find(selectedScenePath)->second;
    }

    auto it = sceneByScenePathMap.find("newScene");
    if (it == sceneByScenePathMap.end())
    {
        LOG_INFO(LogChannel::Editor, "No scene selected; creating a new scene");
        sceneByScenePathMap.insert({"newScene", Scene{std::move("newScene")}});
    }

    return sceneByScenePathMap.find("newScene")->second;
}

bool EditorContext::isRuntimePlaying() const
{
    return runtimeState == RuntimeState::Playing;
}

bool EditorContext::isRuntimeActive() const
{
    return runtimeState != RuntimeState::Stopped;
}

void EditorContext::snapshotScene()
{
    Scene &scene = getActiveScene();
    getSelectedSceneHistory().pushSnapshot(scene);
}

void EditorContext::startEdit()
{
    editingInProgress = true;
    snapshotScene();
}

void EditorContext::endEdit()
{
    editingInProgress = false;
    snapshotScene();
}

void EditorContext::startRuntimeScene()
{
    if (runtimeState != RuntimeState::Stopped)
    {
        return;
    }

    // always insert/assign in playmode
    sceneByScenePathMap.insert_or_assign("runtime_scene", getActiveScene().clone("runtime_scene"));
    runtimeState = RuntimeState::Playing;
    selectedObjects.clear();
    selectedGameObjectsDragOffset.clear();
    getActiveScene().play();
}

void EditorContext::resumeRuntimeScene()
{
    if (runtimeState != RuntimeState::Paused)
    {
        return;
    }

    auto runtimeScene = sceneByScenePathMap.find("runtime_scene");
    if (runtimeScene == sceneByScenePathMap.end())
    {
        std::cerr << "Cannot resume: runtime_scene was not found" << std::endl;
        runtimeState = RuntimeState::Stopped;
        return;
    }

    runtimeScene->second.play();
    runtimeState = RuntimeState::Playing;
}

void EditorContext::pauseRuntimeScene()
{
    if (runtimeState != RuntimeState::Playing)
    {
        return;
    }

    auto runtimeScene = sceneByScenePathMap.find("runtime_scene");
    if (runtimeScene == sceneByScenePathMap.end())
    {
        std::cerr << "Cannot pause: runtime_scene was not found" << std::endl;
        runtimeState = RuntimeState::Stopped;
        return;
    }

    runtimeScene->second.pause();
    runtimeState = RuntimeState::Paused;
}

void EditorContext::stopRuntimeScene()
{
    if (runtimeState == RuntimeState::Stopped)
    {
        return;
    }

    sceneByScenePathMap.erase("runtime_scene");
    runtimeState = RuntimeState::Stopped;
    selectedObjects.clear();
    selectedGameObjectsDragOffset.clear();
}

// TODO: move this to editor layer
void EditorContext::deserializeSelectedScene()
{
    YAML::Node serializedScene = YAML::LoadFile(selectedScenePath);

    // Check if loaded node is valid and not empty
    if (!serializedScene.IsDefined() || serializedScene.size() == 0)
    {
        std::cerr << "scene.yaml is empty or invalid" << std::endl;
        Scene scene{"default_scene"};
        sceneByScenePathMap.insert_or_assign(selectedScenePath, std::move(scene));
        return;
    }

    Scene scene{std::move(serializedScene)};
    sceneByScenePathMap.insert({selectedScenePath, std::move(scene)});
    selectedObjects.clear();
    selectedGameObjectsDragOffset.clear();
    LOG_INFO(LogChannel::Editor, "Deserialized scene from scene.yaml");
}

void EditorContext::serializaActiveScene()
{
    YAML::Emitter out;
    getActiveScene().serialize(out);
    std::string sceneName = getActiveScene().getTag();
    std::string filePath = getGameAssetsPath("scenes/" + sceneName + ".yaml");
    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    file << out.c_str();

    LOG_INFO(LogChannel::Editor, "Serialized scene to '{}.yaml'", sceneName);
}

// TODO: Is there a better place to put this?
void EditorContext::loadEditorWorkspaceState()
{
    const std::filesystem::path statePath = getEditorWorkspaceStatePath();
    if (!std::filesystem::exists(statePath))
    {
        return;
    }

    YAML::Node root;
    try
    {
        root = YAML::LoadFile(statePath.string());
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to load editor workspace state: " << e.what() << std::endl;
        return;
    }

    const YAML::Node panels = root["Panels"];
    if (panels)
    {
        if (panels["Scene"])
            workspaceState.showScenePanel = panels["Scene"].as<bool>();
        if (panels["Hierarchy"])
            workspaceState.showHierarchyPanel = panels["Hierarchy"].as<bool>();
        if (panels["Properties"])
            workspaceState.showPropertiesPanel = panels["Properties"].as<bool>();
        if (panels["Assets"])
            workspaceState.showAssetsPanel = panels["Assets"].as<bool>();
        if (panels["SpriteSheetEditor"])
            workspaceState.showSpriteSheetEditorPanel = panels["SpriteSheetEditor"].as<bool>();
        if (panels["Performance"])
            workspaceState.showPerformancePanel = panels["Performance"].as<bool>();
        if (panels["Systems"])
            workspaceState.showSystemsPanel = panels["Systems"].as<bool>();
        if (panels["Log"])
            workspaceState.showLogPanel = panels["Log"].as<bool>();
        if (panels["Context"])
            workspaceState.showContextPanel = panels["Context"].as<bool>();
        if (panels["Metrics"])
            workspaceState.showMetricsWindow = panels["Metrics"].as<bool>();
    }

    const YAML::Node editor = root["Editor"];
    if (editor)
    {
        if (editor["DrawGrid"])
        {
            drawGrid = editor["DrawGrid"].as<bool>();
        }

        if (editor["SnapToGrid"] && editor["SnapToGrid"].as<bool>())
        {
            movementMode = MovementMode::SnapToGrid;
        }
        else
        {
            movementMode = MovementMode::Free;
        }

        if (editor["SelectedScenePath"])
        {
            const std::string savedScenePath = editor["SelectedScenePath"].as<std::string>();
            if (!savedScenePath.empty() && std::filesystem::exists(savedScenePath))
            {
                selectedScenePath = savedScenePath;
                selectedAssetChildFolderPath = "scenes";
            }
        }
        if (editor["EditorRendererMask"])
            editorRendererMask = editor["EditorRendererMask"].as<uint32_t>();
        if (editor["RuntimeRendererMask"])
            runtimeRendererMask = editor["RuntimeRendererMask"].as<uint32_t>();
    }
}

void EditorContext::saveEditorWorkspaceState()
{
    const std::filesystem::path statePath = getEditorWorkspaceStatePath();
    std::filesystem::create_directories(statePath.parent_path());

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Panels" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << workspaceState.showScenePanel;
    out << YAML::Key << "Hierarchy" << YAML::Value << workspaceState.showHierarchyPanel;
    out << YAML::Key << "Properties" << YAML::Value << workspaceState.showPropertiesPanel;
    out << YAML::Key << "Assets" << YAML::Value << workspaceState.showAssetsPanel;
    out << YAML::Key << "SpriteSheetEditor" << YAML::Value << workspaceState.showSpriteSheetEditorPanel;
    out << YAML::Key << "Performance" << YAML::Value << workspaceState.showPerformancePanel;
    out << YAML::Key << "Systems" << YAML::Value << workspaceState.showSystemsPanel;
    out << YAML::Key << "Log" << YAML::Value << workspaceState.showLogPanel;
    out << YAML::Key << "Context" << YAML::Value << workspaceState.showContextPanel;
    out << YAML::Key << "Metrics" << YAML::Value << workspaceState.showMetricsWindow;
    out << YAML::EndMap;

    out << YAML::Key << "Editor" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "DrawGrid" << YAML::Value << drawGrid;
    out << YAML::Key << "SnapToGrid" << YAML::Value << (movementMode == MovementMode::SnapToGrid);
    out << YAML::Key << "SelectedScenePath" << YAML::Value << selectedScenePath;
    out << YAML::Key << "EditorRendererMask" << YAML::Value << editorRendererMask;
    out << YAML::Key << "RuntimeRendererMask" << YAML::Value << runtimeRendererMask;
    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream file(statePath, std::ios::out | std::ios::trunc);
    file << out.c_str();
    editorWorkspaceDirty = false;
}

void EditorContext::saveEditorWorkspaceStateIfDirty()
{
    if (!editorWorkspaceDirty)
    {
        return;
    }

    saveEditorWorkspaceState();
}

void EditorContext::markEditorWorkspaceDirty()
{
    editorWorkspaceDirty = true;
}

void EditorContext::copySelectedObjectsToClipboard()
{
    clipboardSerializedObjects.clear();

    if (selectedObjects.empty())
    {
        return;
    }

    for (GameObject &go : selectedObjects)
    {
        YAML::Emitter out;
        auto e = go.getComponent<EntityInfo>();
        LOG_DEBUG(LogChannel::Editor, "Copying object with tag '{}'", e.tag);
        go.serialize(out);
        clipboardSerializedObjects.push_back(out.c_str());
    }
}

void EditorContext::pasteClipboardObjects()
{
    if (clipboardSerializedObjects.empty())
    {
        return;
    }

    Scene &scene = getActiveScene();

    // Offset pasted objects slightly so they don't overlap originals and it is
    // visually obvious that a paste occurred.
    const glm::vec2 pasteOffset{16.0f, 16.0f};

    for (const std::string &yaml : clipboardSerializedObjects)
    {
        YAML::Node components = YAML::Load(yaml);
        if (!components || components.size() == 0)
        {
            continue;
        }

        GameObject &newObj = scene.addGameObjectFromSerialized(components);

        if (newObj.hasComponent<Transform>())
        {
            Transform &transform = newObj.getComponent<Transform>();
            transform.position = {
                transform.position.x + pasteOffset.x, transform.position.y + pasteOffset.y, transform.position.z};
        }
    }
}
