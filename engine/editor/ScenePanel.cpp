#include "core/components/Sprite.h"
#include "core/components/Transform.h"

#include "core/AssetManager.h"
#include "core/MouseListener.h"
#include "core/SpriteSheet.h"

#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"
#include "editor/ScenePanel.h"
#include "editor/SpritePayload.h"

#include "util/PathUtils.h"

#include <fstream>
#include <imgui.h>
#include <memory>
#include <nlohmann/json.h>

ScenePanel::ScenePanel(EditorContext &ctx) : EditorPanel(ctx), m_ctx{ctx}
{
}

ScenePanel::~ScenePanel()
{
}

void ScenePanel::draw()
{
    m_ctx.editorMouseController.update(m_ctx);

    renderSceneViewport();
}

void ScenePanel::renderPlayPause()
{
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
}

void ScenePanel::renderMovementMode()
{
    if (ImGui::BeginMenuBar())
    {
        // Selection Mode
        if (ImGui::Button("Select"))
        {
            m_ctx.interactionMode = EditorInteractionMode::Selection;
        }
        ImGui::SameLine();

        // Gliding Mode
        if (ImGui::Button("Glide"))
        {
            m_ctx.interactionMode = EditorInteractionMode::Gliding;
        }
        ImGui::SameLine();

        // Move Mode
        if (ImGui::Button("Move"))
        {
            m_ctx.interactionMode = EditorInteractionMode::MoveObjects;
        }

        ImGui::EndMenuBar();
    }
}

void ScenePanel::renderSceneViewport()
{
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, IM_COL32(0, 0, 0, 255));
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleColor();

    renderPlayPause();
    renderMovementMode();

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
            std::shared_ptr<SpriteSheet> spriteSheet =
                AssetManager::instance().getSpriteSheet(m_ctx.selectedTextureJsonPath);

            int fbWidth, fbHeight;
            glfwGetFramebufferSize(m_ctx.window.getGlfwWindow(), &fbWidth, &fbHeight);

            MouseListener *mouse = MouseListener::instance();
            glm::vec2 worldPos = m_ctx.getWorldCoordinate(mouse->getMouseScreenPosition());

            // Create object here
            Sprite sprite = spriteSheet->getSprites()[spriteIndex];
            const float aspectRatio = sprite.width / sprite.height;
            GameObject &newObj = m_ctx.sceneManager.getActiveScene().addGameObject(32 * aspectRatio, 32, "_new");
            newObj.addComponent<Sprite>(sprite.topLeft, sprite.width, sprite.height, sprite.texture);
            newObj.getComponent<Transform>().position = {worldPos.x, worldPos.y, 0.0f};
        }
        ImGui::EndDragDropTarget();
    }

    m_ctx.scenePanelTopLeftPos = ImGui::GetItemRectMin();
    m_ctx.scenePanelSize = ImGui::GetItemRectSize();

    // render gizmos in the same imgui Begin-End window
    renderGizmos();

    // Store whether this specific image (scene window) is hovered
    // We use this to distinguish mouse clicks in/outside our scene preview
    m_ctx.sceneImageHovered = ImGui::IsItemHovered();

    ImGui::End();
}

void ScenePanel::renderGizmos()
{
    if (m_ctx.selectedObjects.size() != 1)
        return;

    GameObject &go = m_ctx.selectedObjects.back();
    Transform &transform = go.getComponent<Transform>();
    glm::vec3 posCenter = transform.position;

    glm::vec2 screenPos = m_ctx.getScreenCoordinate({posCenter.x, posCenter.y});
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    // Length of gizmo axis
    float axisLength = 50.0f;

    // X-axis (Green)
    ImVec2 xStart = {screenPos.x, screenPos.y};
    ImVec2 xEnd = ImVec2(screenPos.x + axisLength, screenPos.y);
    drawList->AddLine(xStart, xEnd, IM_COL32(0, 255, 0, 255), 2.0f);
    // Arrowhead for X
    drawList->AddTriangleFilled(ImVec2(xEnd.x, xEnd.y), ImVec2(xEnd.x - 6, xEnd.y - 4), ImVec2(xEnd.x - 6, xEnd.y + 4),
        IM_COL32(0, 255, 0, 255));
    // Label for X
    drawList->AddText(ImVec2(xEnd.x + 4, xEnd.y - 6), IM_COL32(0, 255, 0, 255), "X");

    // Y-axis (Blue)
    ImVec2 yStart = {screenPos.x, screenPos.y};
    ImVec2 yEnd = ImVec2(screenPos.x, screenPos.y - axisLength);
    drawList->AddLine(yStart, yEnd, IM_COL32(0, 0, 255, 255), 2.0f);
    // Arrowhead for Y
    drawList->AddTriangleFilled(ImVec2(yEnd.x, yEnd.y), ImVec2(yEnd.x - 4, yEnd.y + 6), ImVec2(yEnd.x + 4, yEnd.y + 6),
        IM_COL32(0, 0, 255, 255));
    // Label for Y
    drawList->AddText(ImVec2(yEnd.x + 4, yEnd.y - 10), IM_COL32(0, 0, 255, 255), "Y");
}
