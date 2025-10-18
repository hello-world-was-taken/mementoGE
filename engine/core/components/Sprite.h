#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "opengl/Texture.h"

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#include <imgui.h>
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
};

#ifdef EDITOR_BUILD
namespace EditorExtensions
{
    void serializeSprite(YAML::Emitter &out, const Sprite &sprite);
    void deserializeSprite(const YAML::Node &in, Sprite &sprite);
    void drawSpriteInspector(Sprite &sprite);
}
#endif
