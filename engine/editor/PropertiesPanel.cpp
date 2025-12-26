#include "core/components/CircleCollider2D.h"
#include "core/components/EnemyState.h"
#include "core/components/EntityInfo.h"
#include "core/components/Patrol.h"
#include "core/components/PostProcessSettings.h"
#include "core/components/RenderLayer.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/Transform.h"

#include "core/AssetManager.h"
#include "core/SpriteSheet.h"
#include "core/components/Animator.h"

#include "editor/DragNDropPayloads.h"
#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"
#include "editor/PropertiesPanel.h"
#include "editor/TexturePanel.h"

#include "util/PathUtils.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>

namespace fs = std::filesystem;

inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

PropertiesPanel::PropertiesPanel(EditorContext &ctx, TexturePanel &texturePanel)
    : EditorPanel{ctx}, m_ctx{ctx}, m_texturePanel{texturePanel}
{
}

PropertiesPanel::~PropertiesPanel()
{
}

void PropertiesPanel::draw()
{
    renderPropertiesInWindow();
    renderPropertiesInPopup();
}

void PropertiesPanel::renderPropertiesInWindow()
{
    ImGui::Begin("Properties");
    renderPropertiesPanel();
    ImGui::End();
}

void PropertiesPanel::renderPropertiesInPopup()
{
    if (!m_ctx.showPropertiesPopup)
    {
        return;
    }

    // Place popup a little to the right of mouse click
    ImGui::SetNextWindowPos(m_ctx.propertiesPopupPos);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("PropertiesPopup"))
    {
        ImGui::BeginChild("PropertiesScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

        renderPropertiesPanel();

        ImGui::EndChild();
        ImGui::EndPopup();
    }
    else
    {
        m_ctx.showPropertiesPopup = false;
    }
}

void PropertiesPanel::renderPropertiesPanel()
{
    if (m_ctx.getActiveScene().getGameObjects().empty())
    {
        return;
    }

    if (m_ctx.selectedObjects.empty())
    {
        ImGui::Text("No game object selected");
        return;
    }

    GameObject &go = m_ctx.selectedObjects.back().get();

    // TODO: look into entt::meta
    drawComponentInspector<EntityInfo>(go);
    drawComponentInspector<Transform>(go);
    drawComponentInspector<RenderLayer>(go);
    drawComponentInspector<BoxCollider2D>(go);
    drawComponentInspector<RigidBody2D>(go);
    drawComponentInspector<EnemyState>(go);
    drawComponentInspector<Patrol>(go);
    drawComponentInspector<Text>(go);
    drawComponentInspector<ParticleEmitter>(go);
    drawComponentInspector<Sensor2D>(go);
    drawComponentInspector<Sprite>(go);
    drawComponentInspector<PostProcessSettings>(go);
    drawComponentInspector<Animator>(go);
    drawAddComponentCombo(go);
    drawExportModel(go);

    ImGui::Separator();
    if (ImGui::Button("Delete Object"))
    {
        m_ctx.getActiveScene().removeGameObject(go.getEntityId());
        m_ctx.selectedGameObjectsDragOffset.clear();
        m_ctx.selectedObjects.clear();
    }

    // Popups (global)
    drawPopups();
}

void PropertiesPanel::drawAddComponentCombo(GameObject &go)
{
    ImGui::Separator();
    SetFieldWidth(150);
    // TODO: using templates might be better than listing the selectables. We have ones that have
    // physics call and one without a simple if constexpr
    // would suffice I think
    if (ImGui::BeginCombo("Add Component", "Select..."))
    {
        if (ImGui::Selectable("Rigidbody2D"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<RigidBody2D>();
                    m_ctx.getActiveScene().getPhysics2d().registerRigidBody2D(go);
                });
        }

        if (ImGui::Selectable("BoxCollider2D"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    EntityInfo &entityInfo = go.getComponent<EntityInfo>();
                    go.addComponent<BoxCollider2D>();
                    go.getComponent<BoxCollider2D>().size = {entityInfo.width, entityInfo.height};
                    m_ctx.getActiveScene().getPhysics2d().registerRigidBody2D(go);
                });
        }

        if (ImGui::Selectable("CircleCollider2D"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<CircleCollider2D>();
                    m_ctx.getActiveScene().getPhysics2d().registerRigidBody2D(go);
                });
        }

        if (ImGui::Selectable("Sensor2D"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Sensor2D>();
                    m_ctx.getActiveScene().getPhysics2d().registerSensor2D(go);
                });
        }

        if (ImGui::Selectable("Animator"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Animator>();
                });
        }

        if (ImGui::Selectable("Enemy Stats"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<EnemyState>();
                    go.getComponent<EnemyState>().startPosition = glm::vec3{go.getComponent<Transform>().position};
                });
        }

        if (ImGui::Selectable("Patrol"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Patrol>();
                });
        }

        if (ImGui::Selectable("Text"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Text>();
                });
        }

        if (ImGui::Selectable("Particle Emitter"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<ParticleEmitter>();
                });
        }

        // TODO: this should only be selected for the camera
        if (ImGui::Selectable("Post Processing Settings"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<PostProcessSettings>();
                });
        }

        ImGui::EndCombo();
    }
}

void PropertiesPanel::drawRigidBodySettings(GameObject &go)
{
    if (!go.hasComponent<RigidBody2D>())
    {
        return;
    }

    RigidBody2D &rb = go.getComponent<RigidBody2D>();
    rb.drawInspector();
}

void PropertiesPanel::drawPopups()
{
    // Missing texture
    if (ImGui::BeginPopupModal("MissingTexturePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Please select a texture first!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void PropertiesPanel::drawExportModel(GameObject &go)
{
    if (ImGui::Button("Export Model"))
    {
        YAML::Emitter out;
        go.serialize(out);

        EntityInfo &info = go.getComponent<EntityInfo>();
        std::ofstream file(getGameAssetsPath("models/" + info.tag + ".yaml"), std::ios::out | std::ios::trunc);
        file << out.c_str();

        std::cout << "Export model: " << info.tag << std::endl;
    }
}