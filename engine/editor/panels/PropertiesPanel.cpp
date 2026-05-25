#include "core/components/AudioSource.h"
#include "core/components/Camera.h"
#include "core/components/CircleCollider2D.h"
#include "core/components/EnemyState.h"
#include "core/components/EntityInfo.h"
#include "core/components/EntityRef.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/Patrol.h"
#include "core/components/PostProcessSettings.h"
#include "core/components/RenderLayer.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"
#include "core/components/TextAnchor.h"
#include "core/components/Transform.h"

#include "core/AssetManager.h"
#include "core/ComponentRegistry.h"
#include "core/SpriteSheet.h"
#include "core/components/Animator.h"

#include "editor/DragNDropPayloads.h"
#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"
#include "editor/panels/PropertiesPanel.h"

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

PropertiesPanel::PropertiesPanel(EditorContext &ctx) : EditorPanel{ctx}, m_ctx{ctx}
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
        ImGui::BeginChild("PropertiesScrollRegion", ImVec2(0, 0), false, ImGuiChildFlags_AlwaysUseWindowPadding);

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

        if (ImGui::Selectable("Audio Source"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<AudioSource>();
                });
        }

        if (ImGui::Selectable("Camera"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<Camera>();
                });
        }

        if (ImGui::Selectable("Entity Ref"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<EntityRef>();
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

        // TODO: should we just add text anchor properties to Text?
        if (ImGui::Selectable("Text Anchor"))
        {
            m_ctx.performSceneEdit(
                [&]
                {
                    go.addComponent<TextAnchor>();
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
        // FIXME: For models, we serialize only the component map (no top-level
        // entity name wrapper) so that it can be loaded directly into
        // GameObject's deserialization path.
        out << YAML::BeginMap;
        go.serializeComponent<EntityInfo>(out);
        go.serializeComponent<Transform>(out);
        go.serializeComponent<RenderLayer>(out);
        go.serializeComponent<Sprite>(out);
        go.serializeComponent<RigidBody2D>(out);
        go.serializeComponent<BoxCollider2D>(out);
        go.serializeComponent<Sensor2D>(out);
        go.serializeComponent<Animator>(out);
        go.serializeComponent<EnemyState>(out);
        go.serializeComponent<Patrol>(out);
        go.serializeComponent<Text>(out);
        go.serializeComponent<TextAnchor>(out);
        go.serializeComponent<ParticleEmitter>(out);
        go.serializeComponent<PostProcessSettings>(out);
        go.serializeComponent<EntityRef>(out);

        // Let game code extend model export with its own components
        // via the same registry used for scene serialization.
        const auto &extraEntries = ComponentRegistry::instance().getEntries();
        for (const auto &entry : extraEntries)
        {
            if (entry.serialize)
            {
                entry.serialize(go, out);
            }
        }
        out << YAML::EndMap;

        EntityInfo &info = go.getComponent<EntityInfo>();
        std::ofstream file(getGameAssetsPath("models/" + info.tag + ".yaml"), std::ios::out | std::ios::trunc);
        file << out.c_str();

        std::cout << "Export model: " << info.tag << std::endl;
    }
}
