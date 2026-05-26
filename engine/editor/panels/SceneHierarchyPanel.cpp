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
#include "core/ImGuiWrapper.h"
#include "core/Scene.h"

#include "editor/EditorContext.h"
#include "editor/panels/SceneHierarchyPanel.h"

#include <imgui.h>

SceneHierarchyPanel::SceneHierarchyPanel(EditorContext &ctx) : EditorPanel(ctx)
{
}

void SceneHierarchyPanel::draw()
{
    if (!m_ctx.workspaceState.showHierarchyPanel)
    {
        return;
    }

    const bool wasOpen = m_ctx.workspaceState.showHierarchyPanel;
    if (!ImGuiWrapper::BeginPanel("Hierarchy", &m_ctx.workspaceState.showHierarchyPanel))
    {
        ImGuiWrapper::EndPanel();
        if (wasOpen != m_ctx.workspaceState.showHierarchyPanel)
        {
            m_ctx.markEditorWorkspaceDirty();
        }
        return;
    }

    drawSceneHierarchy();
    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showHierarchyPanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
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

    ImGuiWrapper::Padding padding{2.0f, 2.0f, 4.0f, 4.0f};
    bool opened = ImGuiWrapper::TreeNodeEx((void *)&go, entityInfo.tag.c_str(), flags, padding);

    // Selection handling
    if (ImGui::IsItemClicked())
    {
        m_ctx.selectedObjects.clear();
        m_ctx.selectedObjects.push_back(go);
    }

    if (opened)
    {
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

            ImGui::PushID(entry.name.c_str());

            if (ImGuiWrapper::Row(entry.name.c_str(), ICON_FA_CUBE, ICON_FA_TIMES))
            {
                entry.removeComponent(go);
            }

            ImGui::PopID();
        }

        ImGuiWrapper::TreePop();
    }
}
