#pragma once

#include <cstdint>
#include <entt/entt.hpp>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

class GameObject;

struct EntityRef
{
    uint64_t targetUuid = 0;
    entt::entity target = entt::null;

    bool resolve(entt::registry &registry);

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &node);
    void drawInspector(GameObject &go);
#endif
};
