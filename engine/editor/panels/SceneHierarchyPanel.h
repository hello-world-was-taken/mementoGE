#pragma once

#include "core/components/EntityInfo.h"
#include "core/components/Transform.h"

#include "core/IconsFontAwesome4.h"

#include "editor/EditorPanel.h"

#include <type_traits>

class SceneHierarchyPanel : public EditorPanel
{
public:
    SceneHierarchyPanel(EditorContext &ctx);
    ~SceneHierarchyPanel() override = default;

    void draw() override;

private:
    void drawSceneHierarchy();
    void drawGameObjectNode(GameObject &go);
    template <typename T> void drawComponentRemoveRow(GameObject &go, const std::string &label);
};

template <typename T> void SceneHierarchyPanel::drawComponentRemoveRow(GameObject &go, const std::string &label)
{
    if (!go.hasComponent<T>())
    {
        return;
    }

    bool removeComponent = false;

    ImGui::PushID(label.c_str());

    ImGui::Text(ICON_FA_SUPERPOWERS "%s", label.c_str());

    if constexpr (std::is_same_v<T, EntityInfo> || std::is_same_v<T, Transform>)
    {
        ImGui::PopID();
        return;
    }

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);

    if (ImGui::SmallButton("X"))
    {
        removeComponent = true;
    }

    if (removeComponent)
    {
        go.removeComponent<T>();
    }

    ImGui::PopID();
}
