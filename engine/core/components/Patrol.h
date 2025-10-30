#pragma once

#ifdef EDITOR_BUILD
#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

struct Patrol
{
    float currentTraveled = 0.0f;
    float moveLeftDistance = 64.0f;
    float moveRightDistance = 64.0f;
    // TODO: change to enum
    bool direction = true; // true: right / false: left
    // TODO: add patrol animation

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &out);
    void drawInspector();
#endif
};