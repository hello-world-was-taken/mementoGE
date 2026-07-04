#pragma once

#include <cstdint>
#include <entt/entt.hpp>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

class GameObject;
class SystemEntities;

struct EntityRef
{
    uint64_t targetUuid = 0;
    entt::entity target = entt::null;

    bool resolve(entt::registry &registry);
    bool resolve(const SystemEntities &entities);

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &node);
    void drawInspector(GameObject &go);
#endif

private:
    template <typename Entities> bool resolveFrom(const Entities &entities);
};
