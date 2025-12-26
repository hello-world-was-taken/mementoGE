#pragma once

#include "core/components/Transform.h"

#include "core/Font.h"

#include "opengl/Vertex.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct Text
{
    std::string content = "Text";
    std::shared_ptr<Font> font;

    float fontSize = 32.0f; // world-space size
    glm::vec4 color = {1, 1, 1, 1};

    float letterSpacing = 0.0f;
    float lineSpacing = 0.0f;

    std::vector<Vertex> vertices;

    bool needsRebuild = true; // TODO: not being used
    void rebuild(Transform &transform);

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};
