#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "opengl/Texture.h"

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct Sprite
{
    glm::vec2 topLeft;
    float width = 0.0f;
    float height = 0.0f;

    std::shared_ptr<Texture> texture;
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // white

    bool flipX = false;
    bool flipY = false;

    // TODO: think more about this
    std::array<glm::vec2, 4> getNormalizedTextureCoordinates() const;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};
