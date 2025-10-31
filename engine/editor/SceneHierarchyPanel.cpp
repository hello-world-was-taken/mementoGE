#include "core/GameObject.h"
#include "core/Scene.h"

#include "editor/EditorContext.h"
#include "editor/SceneHierarchyPanel.h"

#include <imgui.h>

SceneHierarchyPanel::SceneHierarchyPanel(EditorContext &ctx) : EditorPanel(ctx)
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
        m_ctx.selectedObjects.clear();
    }
}

void SceneHierarchyPanel::drawGameObjectNode(GameObject &go)
{
    bool isSelectedGameObject = false;
    if (!m_ctx.selectedObjects.empty())
    {
        GameObject &activeGameObject = m_ctx.selectedObjects.back();
        isSelectedGameObject = activeGameObject.getEntityId() == go.getEntityId();
    }
    ImGuiTreeNodeFlags flags = (isSelectedGameObject ? ImGuiTreeNodeFlags_Selected : 0) |
                               ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    bool opened = ImGui::TreeNodeEx((void *)&go, flags, "%s", go.getTag().c_str());

    // Selection handling
    if (ImGui::IsItemClicked())
    {
        m_ctx.selectedObjects.clear();
        m_ctx.selectedObjects.push_back(go);
    }

    if (opened)
        ImGui::TreePop();
}
