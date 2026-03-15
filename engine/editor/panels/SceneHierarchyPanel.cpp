#include "core/components/AudioSource.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/EnemyState.h"
#include "core/components/EntityInfo.h"
#include "core/components/EntityRef.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/Patrol.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"
#include "core/components/Transform.h"

#include "core/components/Animator.h"

#include "core/ComponentRegistry.h"
#include "core/GameObject.h"
#include "core/Scene.h"

#include "editor/EditorContext.h"
#include "editor/panels/SceneHierarchyPanel.h"

#include <imgui.h>

SceneHierarchyPanel::SceneHierarchyPanel(EditorContext &ctx) : EditorPanel(ctx)
{
}

void SceneHierarchyPanel::draw()
{
    ImGui::Begin("Hierarchy");
    drawSceneHierarchy();
    ImGui::End();
}

void SceneHierarchyPanel::drawSceneHierarchy()
{
    Scene &scene = m_ctx.getActiveScene();
    auto &gameObjects = scene.getGameObjects();

    for (auto &go : gameObjects)
    {
        drawGameObjectNode(go);
    }

    // Allow clicking on empty space to deselect
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
    {
        m_ctx.selectedObjects.clear();
    }
}

void SceneHierarchyPanel::drawGameObjectNode(GameObject &go)
{
    EntityInfo &entityInfo = go.getComponent<EntityInfo>();
    bool isSelectedGameObject = false;
    if (!m_ctx.selectedObjects.empty())
    {
        GameObject &activeGameObject = m_ctx.selectedObjects.back();
        isSelectedGameObject = activeGameObject.getEntityId() == go.getEntityId();
    }
    ImGuiTreeNodeFlags flags = (isSelectedGameObject ? ImGuiTreeNodeFlags_Selected : 0) |
                               ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    bool opened = ImGui::TreeNodeEx((void *)&go, flags, "%s", entityInfo.tag.c_str());

    // Selection handling
    if (ImGui::IsItemClicked())
    {
        m_ctx.selectedObjects.clear();
        m_ctx.selectedObjects.push_back(go);
    }

    if (opened)
    {
        ImGui::Indent();

        drawComponentRemoveRow<EntityInfo>(go, "Entity Info");
        drawComponentRemoveRow<Transform>(go, "Transform");
        drawComponentRemoveRow<Sprite>(go, "Sprite");
        drawComponentRemoveRow<RigidBody2D>(go, "RigidBody2D");
        drawComponentRemoveRow<BoxCollider2D>(go, "BoxCollider2D");
        drawComponentRemoveRow<Sensor2D>(go, "Sensor2D");
        drawComponentRemoveRow<Animator>(go, "Animator");
        drawComponentRemoveRow<AudioSource>(go, "Audio Source");
        drawComponentRemoveRow<EnemyState>(go, "Enemy State");
        drawComponentRemoveRow<Patrol>(go, "Patrol");
        drawComponentRemoveRow<Text>(go, "Text");
        drawComponentRemoveRow<ParticleEmitter>(go, "Particle Emitter");
        drawComponentRemoveRow<EntityRef>(go, "Entity Ref");

        // Let game code provide remove-rows for its own components via
        // the global component registry.
        const auto &extraEntries = ComponentRegistry::instance().getEntries();
        for (const auto &entry : extraEntries)
        {
            if (!entry.removeComponent)
            {
                continue;
            }

            bool removeComponent = false;

            ImGui::PushID(entry.name.c_str());
            ImGui::Text(ICON_FA_SUPERPOWERS "%s", entry.name.c_str());

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
            if (ImGui::SmallButton("X"))
            {
                removeComponent = true;
            }

            if (removeComponent)
            {
                entry.removeComponent(go);
            }

            ImGui::PopID();
        }

        ImGui::Unindent();

        ImGui::TreePop();
    }
}
