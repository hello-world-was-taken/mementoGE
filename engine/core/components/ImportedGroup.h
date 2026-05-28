#pragma once

#include <cstdint>
#include <string>
#include <yaml-cpp/yaml.h>

struct ImportedGroup
{
    uint64_t groupId = 0;
    std::string sourceAsset;

    void serialize(YAML::Emitter &out) const;
    void deserialize(const YAML::Node &in);

#ifdef EDITOR_BUILD
    void drawInspector();
#endif
};
