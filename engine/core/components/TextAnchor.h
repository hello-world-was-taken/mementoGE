#pragma once

#include <glm/glm.hpp>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

// Describes how a Text component should be anchored
// relative to the logical game viewport (see LOGICAL_WIDTH/HEIGHT).
// The Transform of the owning GameObject will be updated so that
// its position lies at the selected anchor point, plus an optional offset.
enum class TextAnchorPoint
{
    TopLeft = 0,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

struct TextAnchor
{
    TextAnchorPoint anchor = TextAnchorPoint::TopLeft;
    // Additional offset in world units from the anchor position.
    glm::vec2 offset{0.0f, 0.0f};

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};
