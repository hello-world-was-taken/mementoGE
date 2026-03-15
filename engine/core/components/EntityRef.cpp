#include "core/components/EntityRef.h"
#include "core/GameObject.h"
#include "core/components/EntityInfo.h"

bool EntityRef::resolve(entt::registry &registry)
{
    if (target != entt::null && registry.valid(target))
    {
        return true;
    }

    if (targetUuid == 0)
    {
        target = entt::null;
        return false;
    }

    auto view = registry.view<EntityInfo>();
    for (auto entity : view)
    {
        auto &info = view.get<EntityInfo>(entity);
        if (info.uuid == targetUuid)
        {
            target = entity;
            return true;
        }
    }

    target = entt::null;
    return false;
}

#ifdef EDITOR_BUILD

#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>

namespace
{
inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}
} // namespace

void EntityRef::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "EntityRef" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "targetUuid" << YAML::Value << targetUuid;
    out << YAML::EndMap;
}

void EntityRef::deserialize(const YAML::Node &node)
{
    if (!node["EntityRef"])
    {
        return;
    }

    const auto &data = node["EntityRef"];
    if (data["targetUuid"])
    {
        targetUuid = data["targetUuid"].as<uint64_t>();
    }
}

void EntityRef::drawInspector(GameObject &go)
{
    entt::registry &registry = go.getRegistry();
    entt::entity self = go.getEntityId();

    ImGuiWrapper::Collapsable("Entity Reference",
        [&]
        {
            std::string currentLabel = "(None)";
            if (targetUuid != 0)
            {
                resolve(registry);
                if (target != entt::null && registry.valid(target))
                {
                    auto &info = registry.get<EntityInfo>(target);
                    currentLabel = info.tag;
                }
                else
                {
                    currentLabel = "Unresolved (" + std::to_string(targetUuid) + ")";
                }
            }

            SetFieldWidth(200.0f);
            if (ImGui::BeginCombo("Target", currentLabel.c_str()))
            {
                // Option to clear the reference
                if (ImGui::Selectable("(None)", targetUuid == 0))
                {
                    targetUuid = 0;
                    target = entt::null;
                }

                // List all entities except self
                auto view = registry.view<EntityInfo>();
                for (auto entity : view)
                {
                    if (entity == self)
                    {
                        continue;
                    }

                    auto &info = view.get<EntityInfo>(entity);
                    std::string label = info.tag + "##" + std::to_string(info.uuid);
                    bool selected = (info.uuid == targetUuid);

                    if (ImGui::Selectable(label.c_str(), selected))
                    {
                        targetUuid = info.uuid;
                        target = entity;
                    }

                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        });
}

#endif
