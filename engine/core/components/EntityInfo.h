#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include <imgui.h>
#endif

struct EntityInfo
{
    std::string tag;
    unsigned int width = 0;
    unsigned int height = 0;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};
