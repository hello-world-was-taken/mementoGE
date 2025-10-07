#include "core/ScenePanel.h"
#include "core/EditorPanel.h"
#include "core/EditorContext.h"
#include "core/SpritePayload.h"
#include "core/SpriteSheet.h"
#include "core/AssetManager.h"
#include "core/Sprite.h"

#include "util/PathUtils.h"

#include <imgui.h>
#include <nlohmann/json.h>
#include <fstream>
#include <memory>

ScenePanel::ScenePanel(EditorContext &ctx) : EditorPanel(ctx), m_ctx{ctx}
{
}

ScenePanel::~ScenePanel()
{
}

void ScenePanel::draw()
{
    m_ctx.editorMouseController
        .Update(
            m_ctx,
            m_upperLeft,
            m_previewAreaSize,
            m_ctx.viewportWidth,
            m_ctx.viewportHeight);

    renderSceneViewport();
}

void ScenePanel::renderSceneViewport()
{
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, IM_COL32(0, 0, 0, 255));
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleColor();
    if (ImGui::BeginMenuBar())
    {
        // Trying to center the components
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float buttonWidth = 40.0f;
        float totalWidth = 3 * buttonWidth + 2 * spacing;

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float startX = (availableWidth - totalWidth) / 2.0f;

        ImGui::SetCursorPosX(startX);

        if (ImGui::Button("PLAY"))
        {
            if (!m_ctx.sceneManager.isPlaying())
            {
                m_ctx.sceneManager.startRuntimeScene();
            }
        }

        if (ImGui::Button("PAUSE"))
        {
            if (m_ctx.sceneManager.isPlaying())
            {
                m_ctx.sceneManager.pauseRuntimeScene();
                m_ctx.sceneManager.m_isPlaying = false;
            }
        }

        if (ImGui::Button("STOP"))
        {
            m_ctx.sceneManager.stopRuntimeScene();
        }

        ImGui::EndMenuBar();
    }

    ImVec2 imGuiWindowSize = ImGui::GetContentRegionAvail(); // logical units
    ImGuiIO &io = ImGui::GetIO();
    int pixelWidth = (int)(imGuiWindowSize.x * io.DisplayFramebufferScale.x + 0.5f);
    int pixelHeight = (int)(imGuiWindowSize.y * io.DisplayFramebufferScale.y + 0.5f);

    // fallback to at least 1x1
    pixelWidth = std::max(1, pixelWidth);
    pixelHeight = std::max(1, pixelHeight);

    m_ctx.frameBuffer.updateSize(pixelWidth, pixelHeight);
    m_ctx.editorCamera.onViewportResize(imGuiWindowSize.x, imGuiWindowSize.y);

    unsigned int framebufferTexture = m_ctx.frameBuffer.getColorTexture();
    ImGui::Image(framebufferTexture, imGuiWindowSize, ImVec2{0, 1}, ImVec2{1, 0});

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SPRITE"))
        {
            IM_ASSERT(payload->DataSize == sizeof(int) || payload->DataSize == sizeof(SpritePayload));
            int spriteIndex = ((SpritePayload *)payload->Data)->spriteIndex;

            // Convert current mouse to world position
            const Camera &cam = m_ctx.editorCamera;

            std::filesystem::path texturePath = getTexturePathFromJson(m_ctx.selectedTextureJsonPath);
            std::shared_ptr<SpriteSheet> spriteSheet = AssetManager::instance().getSpriteSheet(m_ctx.selectedTextureJsonPath);

            int fbWidth, fbHeight;
            glfwGetFramebufferSize(m_ctx.window.getGlfwWindow(), &fbWidth, &fbHeight);

            glm::vec2 worldPos = m_ctx.editorMouseController.getWorldCoordinate(
                cam, m_upperLeft, m_previewAreaSize,
                fbWidth, fbHeight);

            // Create object here
            Sprite sprite = spriteSheet->getSprites()[spriteIndex];
            const float aspectRatio = sprite.getWidth() / sprite.getHeight();
            m_ctx.sceneManager.getActiveScene().addGameObject(32 * aspectRatio, 32, "_new");
            auto newObj = m_ctx.sceneManager.getActiveScene().getActiveGameObject();
            newObj->addComponent<Sprite>(texturePath, sprite.getTextureCoordinates());
            newObj->getComponent<Transform>().setPosition(worldPos.x, worldPos.y, 0.0f);
        }
        ImGui::EndDragDropTarget();
    }

    m_upperLeft = ImGui::GetItemRectMin();
    m_previewAreaSize = ImGui::GetItemRectSize();

    // render gizmos in the same imgui Begin-End window
    renderGizmos();

    // Store whether this specific image (scene window) is hovered
    // We use this to distinguish mouse clicks in/outside our scene preview
    m_ctx.sceneImageHovered = ImGui::IsItemHovered();

    ImGui::End();
}

void ScenePanel::renderGizmos()
{
    if (m_ctx.sceneManager.getActiveScene().getGameObjects().empty())
        return;

    GameObject *go = m_ctx.sceneManager.getActiveScene().getActiveGameObject();
    if (!go)
    {
        // std::cout << "renderGizmos - No active game object selected" << std::endl;
        return;
    }
    Transform &transform = go->getComponent<Transform>();
    glm::vec3 *pos = transform.getPosition();

    glm::vec2 screenPos = getScreenCoordinate({pos->x, pos->y});
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    // Length of gizmo axis
    float axisLength = 50.0f;

    // X-axis (Green)
    ImVec2 xStart = {screenPos.x, screenPos.y};
    ImVec2 xEnd = ImVec2(screenPos.x + axisLength, screenPos.y);
    drawList->AddLine(xStart, xEnd, IM_COL32(0, 255, 0, 255), 2.0f);
    // Arrowhead for X
    drawList->AddTriangleFilled(
        ImVec2(xEnd.x, xEnd.y),
        ImVec2(xEnd.x - 6, xEnd.y - 4),
        ImVec2(xEnd.x - 6, xEnd.y + 4),
        IM_COL32(0, 255, 0, 255));
    // Label for X
    drawList->AddText(ImVec2(xEnd.x + 4, xEnd.y - 6), IM_COL32(0, 255, 0, 255), "X");

    // Y-axis (Blue)
    ImVec2 yStart = {screenPos.x, screenPos.y};
    ImVec2 yEnd = ImVec2(screenPos.x, screenPos.y - axisLength);
    drawList->AddLine(yStart, yEnd, IM_COL32(0, 0, 255, 255), 2.0f);
    // Arrowhead for Y
    drawList->AddTriangleFilled(
        ImVec2(yEnd.x, yEnd.y),
        ImVec2(yEnd.x - 4, yEnd.y + 6),
        ImVec2(yEnd.x + 4, yEnd.y + 6),
        IM_COL32(0, 0, 255, 255));
    // Label for Y
    drawList->AddText(ImVec2(yEnd.x + 4, yEnd.y - 10), IM_COL32(0, 0, 255, 255), "Y");
}

glm::vec2 ScenePanel::getScreenCoordinate(glm::vec2 worldPos)
{
    glm::vec2 frameBufferPos = worldToFrameBuffer(worldPos);
    glm::vec2 localPos = frameBufferToLocal(frameBufferPos);

    return localToScreen(localPos);
}

glm::vec2 ScenePanel::worldToFrameBuffer(glm::vec2 worldPos)
{
    const Camera &camera = m_ctx.sceneManager.getActiveScene().getCamera();
    glm::mat4 viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();

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
    framebufferCoords.x = (ndcCoords.x + 1.0f) * 0.5f * m_ctx.viewportWidth;
    framebufferCoords.y = (1.0f - ndcCoords.y) * 0.5f * m_ctx.viewportHeight; // flip Y-axis for imgui

    // std::cout << "World Pos: (" << worldPos.x << ", " << worldPos.y << std::endl;
    // std::cout << "Framebuffer Coords: (" << framebufferCoords.x << ", " << framebufferCoords.y << std::endl;

    return framebufferCoords;
}

glm::vec2 ScenePanel::frameBufferToLocal(glm::vec2 frameBufferPos)
{
    float localPosX = (frameBufferPos.x / m_ctx.viewportWidth) * m_previewAreaSize.x;
    float localPosY = (frameBufferPos.y / m_ctx.viewportHeight) * m_previewAreaSize.y;

    return {localPosX, localPosY};
}

glm::vec2 ScenePanel::localToScreen(glm::vec2 localPos)
{
    glm::vec2 screenPos = localPos + glm::vec2{m_upperLeft.x, m_upperLeft.y};

    return screenPos;
}