#include "core/components/Sprite.h"
#include "core/components/Transform.h"

#include "core/IconsFontAwesome4.h"

#include "core/AssetManager.h"
#include "core/MouseListener.h"
#include "core/SpriteSheet.h"

#include "editor/DragNDropPayloads.h"
#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"
#include "editor/panels/ScenePanel.h"

#include "util/PathUtils.h"

#include <fstream>
#include <imgui.h>
#include <memory>
#include <nlohmann/json.h>
#include <string>

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

        if (ImGui::Button(ICON_FA_PLAY))
        {
            if (!m_ctx.isPlaying)
            {
                m_ctx.startRuntimeScene();
                m_ctx.isPlaying = true;
            }
        }

        if (ImGui::Button(ICON_FA_PAUSE))
        {
            if (m_ctx.isPlaying)
            {
                m_ctx.pauseRuntimeScene();
                m_ctx.isPlaying = false;
            }
        }

        if (ImGui::Button(ICON_FA_STOP))
        {
            m_ctx.stopRuntimeScene();
        }

        ImGui::EndMenuBar();
    }
}

void ScenePanel::renderMovementMode()
{
    auto EditorInteractionModeSelectable =
        [&](std::string_view label, EditorInteractionMode mode, std::string_view tooltip)
    {
        auto selectedColor = ImVec4(0.314f, 0.447f, 0.655f, 1.00f);

        // check current state (before clicking)
        bool wasSelected = m_ctx.interactionMode == mode;

        if (wasSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, selectedColor);
        }

        bool selectedOnClick = wasSelected;
        if (ImGui::Selectable(label.begin(), &selectedOnClick, 0, ImVec2(13, 0)))
        {
            if (m_ctx.interactionMode == mode)
            {
                // Deselect back to no explicit interaction mode
                m_ctx.interactionMode = EditorInteractionMode::None;
                m_ctx.baseInteractionMode = EditorInteractionMode::None;
            }
            else
            {
                // Update both the base (persistent) mode and the current mode
                m_ctx.baseInteractionMode = mode;
                m_ctx.interactionMode = mode;
            }
        }

        if (wasSelected)
        {
            ImGui::PopStyleColor(2);
        }

        if (ImGui::IsItemHovered() && !tooltip.empty())
        {
            ImGui::SetTooltip("%s", tooltip.begin());
        }
    };

    if (ImGui::BeginMenuBar())
    {
        ImGui::Spacing();

        // Selection Mode (Default)
        EditorInteractionModeSelectable(
            ICON_FA_MOUSE_POINTER, EditorInteractionMode::Selection, "Select and manipulate objects");
        ImGui::SameLine();

        // Gliding/Panning Mode
        EditorInteractionModeSelectable(
            ICON_FA_HAND_PAPER_O, EditorInteractionMode::Gliding, "Glide through the scene");
        ImGui::SameLine();

        // Move Objects Mode
        EditorInteractionModeSelectable(
            ICON_FA_ARROWS_ALT, EditorInteractionMode::MoveObjects, "Move selected objects");

        ImGui::EndMenuBar();
    }
}

void ScenePanel::renderSceneViewport()
{
    std::string activeSceneName = m_ctx.getActiveScene().getTag();
    std::string visibleTitle = activeSceneName + (m_ctx.getSelectedSceneHistory().isDirty() ? "*" : "");

    // ImGui window identifiers are determined by everything after the ###
    std::string titleWithWindowId = visibleTitle + "###ScenePanel";
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, IM_COL32(0, 0, 0, 255));
    ImGui::Begin(titleWithWindowId.c_str(), nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleColor();

    renderPlayPause();
    renderMovementMode();

    // viewport size + framebuffer update
    ImVec2 imGuiWindowSize = ImGui::GetContentRegionAvail();
    ImGuiIO &io = ImGui::GetIO();
    int pixelWidth = std::max(1, (int)(imGuiWindowSize.x * io.DisplayFramebufferScale.x + 0.5f));
    int pixelHeight = std::max(1, (int)(imGuiWindowSize.y * io.DisplayFramebufferScale.y + 0.5f));

    m_ctx.renderer2D.queueFrameBufferResize(pixelWidth, pixelHeight);
    m_ctx.editorCamera.onViewportResize(imGuiWindowSize.x, imGuiWindowSize.y);

    // render framebuffer texture
    unsigned int framebufferTexture = m_ctx.renderer2D.getColorTexture();
    ImGui::Image(framebufferTexture, imGuiWindowSize, ImVec2{0, 1}, ImVec2{1, 0});

    // handle drag–drop
    handleViewportDropTarget();

    m_ctx.scenePanelTopLeftPos = ImGui::GetItemRectMin();
    m_ctx.scenePanelSize = ImGui::GetItemRectSize();
    m_ctx.sceneImageHovered = ImGui::IsItemHovered();

    if (!m_ctx.isPlaying)
    {
        renderGizmos();
    }

    ImGui::End();
}

void ScenePanel::handleViewportDropTarget()
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *spritePayload = ImGui::AcceptDragDropPayload("SPRITE"))
        {
            m_ctx.performSceneEdit(
                [&]()
                {
                    createSpriteFromPayload(spritePayload);
                });
        }

        if (const ImGuiPayload *modelPayload = ImGui::AcceptDragDropPayload("MODEL"))
        {
            m_ctx.performSceneEdit(
                [&]()
                {
                    createModelFromPayload(modelPayload);
                });
        }

        ImGui::EndDragDropTarget();
    }
}

void ScenePanel::createSpriteFromPayload(const ImGuiPayload *payload)
{
    IM_ASSERT(payload->DataSize == sizeof(SpritePayload));
    SpritePayload spritePayload = *(SpritePayload *)payload->Data;
    MouseListener *mouse = MouseListener::instance();
    glm::vec2 worldPos = m_ctx.getWorldCoordinate(mouse->getMouseScreenPosition());

    const float aspectRatio = spritePayload.width / spritePayload.height;

    GameObject &newObj = m_ctx.getActiveScene().addGameObject(32 * aspectRatio, 32, "_new");
    newObj.addComponent<Sprite>(
        spritePayload.topLeft, spritePayload.width, spritePayload.height, spritePayload.texture);
    newObj.getComponent<Transform>().position = {worldPos.x, worldPos.y, 0.0f};
}

void ScenePanel::createModelFromPayload(const ImGuiPayload *payload)
{
    IM_ASSERT(payload->DataSize == sizeof(ModelPayload));
    ModelPayload modelPayload = *(ModelPayload *)payload->Data;

    MouseListener *mouse = MouseListener::instance();
    glm::vec2 worldPos = m_ctx.getWorldCoordinate(mouse->getMouseScreenPosition());

    Scene &scene = m_ctx.getActiveScene();
    GameObject &newObj = scene.addGameObjectFromModel(modelPayload.filePath);
    newObj.getComponent<Transform>().position = {worldPos.x, worldPos.y, 0.0f};
}

void ScenePanel::renderGizmos()
{
    if (m_ctx.selectedObjects.size() != 1)
    {
        return;
    }

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
    drawList->AddTriangleFilled(ImVec2(xEnd.x, xEnd.y),
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
    drawList->AddTriangleFilled(ImVec2(yEnd.x, yEnd.y),
        ImVec2(yEnd.x - 4, yEnd.y + 6),
        ImVec2(yEnd.x + 4, yEnd.y + 6),
        IM_COL32(0, 0, 255, 255));
    // Label for Y
    drawList->AddText(ImVec2(yEnd.x + 4, yEnd.y - 10), IM_COL32(0, 0, 255, 255), "Y");
}
