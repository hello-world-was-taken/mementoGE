#include "core/components/AudioSource.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/Camera.h"
#include "core/components/CircleCollider2D.h"
#include "core/components/EnemyState.h"
#include "core/components/EntityInfo.h"
#include "core/components/EntityRef.h"
#include "core/components/ImportedGroup.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/Patrol.h"
#include "core/components/PostProcessSettings.h"
#include "core/components/RenderLayer.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/ShaderEffectRequest.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"
#include "core/components/TextAnchor.h"
#include "core/components/Transform.h"

#include "core/AssetManager.h"
#include "core/ComponentRegistry.h"
#include "core/ImGuiWrapper.h"
#include "core/SpriteSheet.h"
#include "core/components/Animator.h"

#include "editor/DragNDropPayloads.h"
#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"
#include "editor/panels/PropertiesPanel.h"

#include "util/LogChannels.h"
#include "util/PathUtils.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>
#include <sawyer/Logger.h>

namespace fs = std::filesystem;

inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

PropertiesPanel::PropertiesPanel(EditorContext &ctx) : EditorPanel{ctx}, m_ctx{ctx}
{
}

PropertiesPanel::~PropertiesPanel()
{
}

void PropertiesPanel::draw()
{
    renderPropertiesInWindow();
    renderPropertiesPopupHost();
}

void PropertiesPanel::renderPropertiesInWindow()
{
    if (!m_ctx.workspaceState.showPropertiesPanel)
    {
        return;
    }

    const bool wasOpen = m_ctx.workspaceState.showPropertiesPanel;
    if (!ImGuiWrapper::BeginPanel("Properties", &m_ctx.workspaceState.showPropertiesPanel))
    {
        ImGuiWrapper::EndPanel();
        if (wasOpen != m_ctx.workspaceState.showPropertiesPanel)
        {
            m_ctx.markEditorWorkspaceDirty();
        }
        return;
    }

    renderPropertiesPanel();
    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showPropertiesPanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
}

void PropertiesPanel::renderPropertiesPopupHost()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(1.0f, 1.0f), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;

    if (ImGui::Begin("PropertiesPopupHost", nullptr, flags))
    {
        renderPropertiesInPopup();
    }
    ImGui::End();
}

void PropertiesPanel::renderPropertiesInPopup()
{
    if (m_ctx.showPropertiesPopup)
    {
        ImGui::SetNextWindowPos(m_ctx.propertiesPopupPos);
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Appearing);
        ImGui::OpenPopup("PropertiesPopup");
        m_ctx.showPropertiesPopup = false;
    }

    if (ImGui::BeginPopup("PropertiesPopup"))
    {
        ImGui::BeginChild("PropertiesScrollRegion", ImVec2(0, 0), false, ImGuiChildFlags_AlwaysUseWindowPadding);

        renderPropertiesPanel();

        ImGui::EndChild();
        ImGui::EndPopup();
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

    GameObject &go = m_ctx.selectedObjects.back();

    // TODO: look into entt::meta
    drawComponentInspector<EntityInfo>(go);
    drawComponentInspector<Transform>(go);
    drawComponentInspector<RenderLayer>(go);
    drawComponentInspector<BoxCollider2D>(go);
    drawComponentInspector<RigidBody2D>(go);
    drawComponentInspector<EnemyState>(go);
    drawComponentInspector<Patrol>(go);
    drawComponentInspector<Text>(go);
    drawComponentInspector<TextAnchor>(go);
    drawComponentInspector<ParticleEmitter>(go);
    drawComponentInspector<Sensor2D>(go);
    drawComponentInspector<Sprite>(go);
    drawComponentInspector<PostProcessSettings>(go);
    drawComponentInspector<Animator>(go);
    drawComponentInspector<AudioSource>(go);
    drawComponentInspector<Camera>(go);
    drawComponentInspector<EntityRef>(go);
    drawComponentInspector<ImportedGroup>(go);
    drawComponentInspector<ShaderEffectRequest>(go);

    // Let game code draw inspectors for its own components via the
    // global component registry.
    const auto &extraEntries = ComponentRegistry::instance().getEntries();
    for (const auto &entry : extraEntries)
    {
        if (entry.drawInspector)
        {
            entry.drawInspector(go);
        }
    }
    drawAddComponentCombo(go);
    drawExportModel(go);

    ImGui::Separator();
    if (ImGuiWrapper::Button("Delete Object"))
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
    if (ImGui::BeginCombo("Add Component", "Select..."))
    {
        if (!go.hasComponent<RigidBody2D>() && ImGui::Selectable("Rigidbody2D"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<RigidBody2D>();
                    m_ctx.getActiveScene().getPhysics2d().registerEntity(go.getRegistry(), go.getEntityId());
                });
        }

        if (!go.hasComponent<BoxCollider2D>() && ImGui::Selectable("BoxCollider2D"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    EntityInfo &entityInfo = go.getComponent<EntityInfo>();
                    go.addComponent<BoxCollider2D>();
                    auto &collider = go.getComponent<BoxCollider2D>();
                    collider.shapes.front().size = {entityInfo.width * 0.75f, entityInfo.height * 0.85f};
                    m_ctx.getActiveScene().getPhysics2d().registerEntity(go.getRegistry(), go.getEntityId());
                });
        }

        if (!go.hasComponent<CircleCollider2D>() && ImGui::Selectable("CircleCollider2D"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<CircleCollider2D>();
                    m_ctx.getActiveScene().getPhysics2d().registerEntity(go.getRegistry(), go.getEntityId());
                });
        }

        if (!go.hasComponent<Sensor2D>() && ImGui::Selectable("Sensor2D"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Sensor2D>();
                    m_ctx.getActiveScene().getPhysics2d().registerEntity(go.getRegistry(), go.getEntityId());
                });
        }

        if (!go.hasComponent<Sprite>() && ImGui::Selectable("Sprite"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Sprite>();
                });
        }

        if (!go.hasComponent<Animator>() && ImGui::Selectable("Animator"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Animator>();
                });
        }

        if (!go.hasComponent<AudioSource>() && ImGui::Selectable("Audio Source"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<AudioSource>();
                });
        }

        if (!go.hasComponent<Camera>() && ImGui::Selectable("Camera"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Camera>();
                });
        }

        if (!go.hasComponent<EntityRef>() && ImGui::Selectable("Entity Ref"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<EntityRef>();
                });
        }

        if (!go.hasComponent<EnemyState>() && ImGui::Selectable("Enemy Stats"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<EnemyState>();
                    go.getComponent<EnemyState>().startPosition = glm::vec3{go.getComponent<Transform>().position};
                });
        }

        if (!go.hasComponent<Patrol>() && ImGui::Selectable("Patrol"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Patrol>();
                });
        }

        if (!go.hasComponent<Text>() && ImGui::Selectable("Text"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Text>();
                });
        }

        // TODO: should we just add text anchor properties to Text?
        if (!go.hasComponent<TextAnchor>() && ImGui::Selectable("Text Anchor"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<TextAnchor>();
                });
        }

        if (!go.hasComponent<ParticleEmitter>() && ImGui::Selectable("Particle Emitter"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<ParticleEmitter>();
                });
        }

        // Game-specific components registered via the component registry.
        const auto &extraEntries = ComponentRegistry::instance().getEntries();
        for (const auto &entry : extraEntries)
        {
            if (!entry.addComponent)
            {
                continue;
            }

            if (ImGui::Selectable(entry.name.c_str()))
            {
                m_ctx.performSceneEdit(
                    [&]
                    {
                        entry.addComponent(go);
                    });
            }
        }

        if (!go.hasComponent<PostProcessSettings>() && ImGui::Selectable("Post Processing Settings"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<PostProcessSettings>();
                });
        }

        if (!go.hasComponent<ImportedGroup>() && ImGui::Selectable("Imported Group"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<ImportedGroup>();
                });
        }

        if (!go.hasComponent<ShaderEffectRequest>() && ImGui::Selectable("Shader Effect"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<ShaderEffectRequest>();
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
        if (ImGuiWrapper::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void PropertiesPanel::drawExportModel(GameObject &go)
{
    if (ImGuiWrapper::Button("Export Model"))
    {
        YAML::Emitter out;
        go.serialize(out);

        EntityInfo &info = go.getComponent<EntityInfo>();
        std::ofstream file(getGameAssetPath("models/" + info.tag + ".yaml"), std::ios::out | std::ios::trunc);
        file << out.c_str();

        LOG_INFO(LogChannel::Editor, "Exported model '{}'", info.tag);
    }
}
