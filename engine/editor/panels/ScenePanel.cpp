#include "core/components/Sprite.h"
#include "core/TileMap.h"
#include "core/components/Transform.h"

#include "core/IconsFontAwesome4.h"

#include "core/AssetManager.h"
#include "core/EventHandler.h"
#include "core/ImGuiWrapper.h"
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
#include <yaml-cpp/yaml.h>

ScenePanel::ScenePanel(EditorContext &ctx) : EditorPanel(ctx), m_ctx{ctx}
{
}

ScenePanel::~ScenePanel()
{
}

void ScenePanel::draw()
{
    if (!m_ctx.workspaceState.showScenePanel)
    {
        return;
    }

    renderSceneViewport();
}

void ScenePanel::renderPlayPause()
{
    if (ImGui::BeginMenuBar())
    {
        // Trying to center the components
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float buttonWidth = 13.0f;
        float totalWidth = 3 * buttonWidth + 2 * spacing;

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float startX = (availableWidth - totalWidth) / 2.0f;

        ImGui::SetCursorPosX(startX);

        if (ImGuiWrapper::ToolbarSelectable(ICON_FA_PLAY,
                m_ctx.runtimeState == RuntimeState::Playing,
                "Play / resume",
                ImVec2(buttonWidth, 0.0f)))
        {
            if (m_ctx.runtimeState == RuntimeState::Stopped)
            {
                m_ctx.startRuntimeScene();
            }
            else if (m_ctx.runtimeState == RuntimeState::Paused)
            {
                m_ctx.resumeRuntimeScene();
            }
        }

        if (ImGuiWrapper::ToolbarSelectable(ICON_FA_PAUSE,
                m_ctx.runtimeState == RuntimeState::Paused,
                "Pause",
                ImVec2(buttonWidth, 0.0f)))
        {
            if (m_ctx.runtimeState == RuntimeState::Playing)
            {
                m_ctx.pauseRuntimeScene();
            }
        }

        if (ImGuiWrapper::ToolbarSelectable(ICON_FA_STOP,
                m_ctx.runtimeState == RuntimeState::Stopped,
                "Stop",
                ImVec2(buttonWidth, 0.0f)))
        {
            if (m_ctx.isRuntimeActive())
            {
                m_ctx.stopRuntimeScene();
            }
        }

        ImGui::EndMenuBar();
    }
}

void ScenePanel::renderMovementMode()
{
    auto EditorInteractionModeSelectable =
        [&](std::string_view label, EditorInteractionMode mode, std::string_view tooltip)
    {
        // check current state (before clicking)
        bool wasSelected = m_ctx.interactionMode == mode;

        if (ImGuiWrapper::ToolbarSelectable(label.data(), wasSelected, tooltip.data()))
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
    const bool wasOpen = m_ctx.workspaceState.showScenePanel;
    if (!ImGuiWrapper::BeginPanel(
            titleWithWindowId.c_str(), &m_ctx.workspaceState.showScenePanel, ImGuiWindowFlags_MenuBar))
    {
        ImGuiWrapper::EndPanel();
        if (wasOpen != m_ctx.workspaceState.showScenePanel)
        {
            m_ctx.markEditorWorkspaceDirty();
        }
        return;
    }

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
    EventHandler::instance()->setSceneViewportBounds(
        {m_ctx.scenePanelTopLeftPos.x, m_ctx.scenePanelTopLeftPos.y},
        {m_ctx.scenePanelTopLeftPos.x + m_ctx.scenePanelSize.x,
            m_ctx.scenePanelTopLeftPos.y + m_ctx.scenePanelSize.y});

    m_ctx.editorMouseController.update(m_ctx);
    renderCreateObjectPopup();

    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showScenePanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
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

void ScenePanel::renderCreateObjectPopup()
{
    if (m_ctx.showCreateObjectPopup)
    {
        ImGui::SetNextWindowPos(m_ctx.createObjectPopupPos, ImGuiCond_Always);
        ImGui::OpenPopup("CreateObjectPopup");
        m_ctx.showCreateObjectPopup = false;
    }

    if (ImGui::BeginPopup("CreateObjectPopup"))
    {
        static char tagBuffer[64] = "";

        ImGui::Text("Create New Object");
        ImGui::Separator();
        ImGui::InputText("Tag", tagBuffer, IM_ARRAYSIZE(tagBuffer));

        if (ImGuiWrapper::Button("Create"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    Scene &scene = m_ctx.getActiveScene();
                    auto &newObj = scene.addGameObject(32, 32, tagBuffer);
                    newObj.getComponent<Transform>().position = {
                        m_ctx.createObjectWorldPos.x, m_ctx.createObjectWorldPos.y, 0.0f};

                    tagBuffer[0] = '\0';
                    m_ctx.showCreateObjectPopup = false;
                    ImGui::CloseCurrentPopup();
                });
        }

        ImGui::SameLine();
        if (ImGuiWrapper::Button("Cancel"))
        {
            m_ctx.showCreateObjectPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ScenePanel::createSpriteFromPayload(const ImGuiPayload *payload)
{
    IM_ASSERT(payload->DataSize == sizeof(SpritePayload));
    SpritePayload spritePayload = *(SpritePayload *)payload->Data;
    glm::vec2 mousePos{0.0f};
    for (const Event &event : m_ctx.frame.input.events)
    {
        if (event.type == EventType::MousePosition)
        {
            mousePos = {event.mouseX, event.mouseY};
        }
    }
    glm::vec2 worldPos = m_ctx.getWorldCoordinate(mousePos);

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

    glm::vec2 mousePos{0.0f};
    for (const Event &event : m_ctx.frame.input.events)
    {
        if (event.type == EventType::MousePosition)
        {
            mousePos = {event.mouseX, event.mouseY};
        }
    }
    glm::vec2 worldPos = m_ctx.getWorldCoordinate(mousePos);

    Scene &scene = m_ctx.getActiveScene();
    const YAML::Node asset = YAML::LoadFile(modelPayload.filePath);
    if (TileMap::isTileMapAsset(asset))
    {
        TileMapImportContext context;
        context.importTransform.position = {worldPos.x, worldPos.y, 0.0f};
        context.sourceAsset = modelPayload.filePath;
        context.createSceneObject =
            [&scene](const YAML::Node &object) -> GameObject & { return scene.addGameObjectFromSerialized(object); };
        TileMap::flattenIntoScene(asset, context);
        return;
    }

    GameObject &newObj = scene.addGameObjectFromSerialized(asset);
    newObj.getComponent<Transform>().position = {worldPos.x, worldPos.y, 0.0f};
}
