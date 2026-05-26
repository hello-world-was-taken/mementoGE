#pragma once

#include "core/components/EntityInfo.h"
#include "core/components/Transform.h"

#include "core/IconsFontAwesome4.h"
#include "core/ImGuiWrapper.h"

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

    ImGui::PushID(label.c_str());

    if constexpr (std::is_same_v<T, EntityInfo> || std::is_same_v<T, Transform>)
    {
        ImGuiWrapper::Row(label.c_str(), ICON_FA_CUBE, ICON_FA_LOCK, false);
        ImGui::PopID();
        return;
    }

    if (ImGuiWrapper::Row(label.c_str(), ICON_FA_CUBE, ICON_FA_TIMES))
    {
        go.removeComponent<T>();
    }

    ImGui::PopID();
}
