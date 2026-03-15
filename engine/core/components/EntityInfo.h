#pragma once

#include <cstdint>
#include <string>
#include <yaml-cpp/yaml.h>

struct EntityInfo
{
    uint64_t uuid = generateUUID();
    std::string tag;
    unsigned int width = 0;
    unsigned int height = 0;

    static uint64_t generateUUID();

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};
