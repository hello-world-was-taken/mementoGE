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

#include <fstream>

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
        std::cout << "Warning: Invalid transformation, w = 0!" << std::endl;
        return glm::vec2(0.0f, 0.0f);
    }

    // convert clip space to normalized device coordinates (NDC)
    glm::vec3 ndcCoords = clipSpaceCoords / clipSpaceCoords.w;

    // map NDC to screen space (viewport coordinates)
    glm::vec2 framebufferCoords;
    framebufferCoords.x =
        (ndcCoords.x + 1.0f) * 0.5f * viewportWidth; // TODO: shouldn't we use framebuffer width/height here?
    framebufferCoords.y = (1.0f - ndcCoords.y) * 0.5f * viewportHeight; // flip Y-axis for imgui

    // std::cout << "World Pos: (" << worldPos.x << ", " << worldPos.y << std::endl;
    // std::cout << "Framebuffer Coords: (" << framebufferCoords.x << ", " << framebufferCoords.y << std::endl;

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
    if (isPlaying)
    {
        auto it = sceneByScenePathMap.find("runtime_scene");
        if (it != sceneByScenePathMap.end())
        {
            return it->second;
        }
        else
        {
            std::cerr << "inPlaying but runtime_scene not found" << std::endl;
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
        std::cout << "getActiveScene called, but no scene is selected. Creating new scene" << std::endl;
        sceneByScenePathMap.insert({"newScene", Scene{std::move("newScene")}});
    }

    return sceneByScenePathMap.find("newScene")->second;
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
    // always insert/assign in playmode
    sceneByScenePathMap.insert_or_assign("runtime_scene", getActiveScene().clone("runtime_scene"));
    isPlaying = true;
    selectedObjects.clear();
    selectedGameObjectsDragOffset.clear();
    getActiveScene().play();
}

void EditorContext::pauseRuntimeScene()
{
    getActiveScene().pause();
    isPlaying = false;
}

void EditorContext::stopRuntimeScene()
{
    sceneByScenePathMap.erase("runtime_scene");
    isPlaying = false;
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
    std::cout << "Deserialized scene from scene.yaml" << std::endl;
}

void EditorContext::serializaActiveScene()
{
    YAML::Emitter out;
    getActiveScene().serialize(out);
    std::string sceneName = getActiveScene().getTag();
    std::string filePath = getGameAssetsPath("scenes/" + sceneName + ".yaml");
    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    file << out.c_str();

    std::cout << "Serialized scene to: " << sceneName << ".yaml" << std::endl;
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
        std::cout << "Tag: " << e.tag << std::endl;
        // Serialize just the component map (no top-level tag), using the
        // same pattern as PropertiesPanel::drawExportModel so that copy /
        // paste and model export stay in sync. If this format changes,
        // FIXME: update both places or extract a shared helper.
        out << YAML::BeginMap;
        go.serializeComponent<EntityInfo>(out);
        go.serializeComponent<Transform>(out);
        go.serializeComponent<RenderLayer>(out);
        go.serializeComponent<Sprite>(out);
        go.serializeComponent<RigidBody2D>(out);
        go.serializeComponent<BoxCollider2D>(out);
        go.serializeComponent<Sensor2D>(out);
        go.serializeComponent<Animator>(out);
        go.serializeComponent<AudioSource>(out);
        go.serializeComponent<EnemyState>(out);
        go.serializeComponent<Patrol>(out);
        go.serializeComponent<Text>(out);
        go.serializeComponent<TextAnchor>(out);
        go.serializeComponent<ParticleEmitter>(out);
        go.serializeComponent<Camera>(out);
        go.serializeComponent<PostProcessSettings>(out);

        const auto &extraEntries = ComponentRegistry::instance().getEntries();
        for (const auto &entry : extraEntries)
        {
            if (entry.serialize)
            {
                entry.serialize(go, out);
            }
        }
        out << YAML::EndMap;
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
