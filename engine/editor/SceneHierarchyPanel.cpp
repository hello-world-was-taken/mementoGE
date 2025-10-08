#include "core/Scene.h"
#include "core/GameObject.h"

#include "editor/SceneHierarchyPanel.h"
#include "editor/EditorContext.h"

#include <imgui.h>

SceneHierarchyPanel::SceneHierarchyPanel(EditorContext &ctx)
    : EditorPanel(ctx)
{
}

void SceneHierarchyPanel::draw()
{
    ImGui::Begin("Scene Hierarchy");
    drawSceneHierarchy();
    ImGui::End();
}

void SceneHierarchyPanel::drawSceneHierarchy()
{
    Scene &scene = m_ctx.sceneManager.getActiveScene();
    auto &gameObjects = scene.getGameObjects();

    for (auto &go : gameObjects)
    {
        drawGameObjectNode(go);
    }

    // Allow clicking on empty space to deselect
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
    {
        m_ctx.sceneManager.getActiveScene().setActiveGameObject(entt::null);
    }
}

void SceneHierarchyPanel::drawGameObjectNode(const GameObject &go)
{
    ImGuiTreeNodeFlags flags =
        ((m_ctx.sceneManager.getActiveScene().getActiveGameObject() == &go) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    bool opened = ImGui::TreeNodeEx((void *)&go, flags, "%s", go.getTag().c_str());

    // Selection handling
    if (ImGui::IsItemClicked())
    {
        m_ctx.sceneManager.getActiveScene().setActiveGameObject(go.getEntityId());
    }

    if (opened)
        ImGui::TreePop();
}
