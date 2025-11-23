#include "editor/EditorContext.h"

#include "util/PathUtils.h"

#include <fstream>

glm::vec2 EditorContext::getWorldCoordinate(glm::vec2 mouseScreenCoords)
{
    glm::vec2 localPos = screenToLocal(mouseScreenCoords);
    glm::vec2 framebufferPos = localToFrameBuffer(localPos);

    // Make sure the mouse is inside the image
    if (localPos.x < 0 || localPos.y < 0 || localPos.x > scenePanelSize.x || localPos.y > scenePanelSize.y)
        return glm::vec2(-1.0f);

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
    float fbX = (localPos.x / scenePanelSize.x) * frameBuffer.getWidth();
    float fbY = (localPos.y / scenePanelSize.y) * frameBuffer.getHeight();

    return {fbX, fbY};
}

glm::vec2 EditorContext::frameBufferToWorld(glm::vec2 fbPos)
{
    // Convert to Normalized Device Coordinates (NDC)
    float ndcX = (fbPos.x / frameBuffer.getWidth()) * 2.0f - 1.0f;
    float ndcY = (fbPos.y / frameBuffer.getHeight()) * 2.0f - 1.0f;

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
