#pragma once

#include "opengl/Texture.h"

#include <glm/glm.hpp>
#include <memory>
#include <random>
#include <vector>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct Sprite
{
    glm::vec2 topLeft;
    float width = 0.0f;
    float height = 0.0f;

    // TODO: why not just a reference?
    std::shared_ptr<Texture> texture;
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // white

    bool flipX = false;
    bool flipY = false;

    // TODO: think more about this
    std::array<glm::vec2, 4> getNormalizedTextureCoordinates() const;

    // WARNING: do not use id direclty. Use getId();
    // TODO: update sprite creation and make this private
    int id = -1;

    int getId();

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();

    void handleSpriteDrop();
#endif
};
