#pragma once

#include "opengl/Texture.h"

#include <memory>
#include <string>

struct SpritePayload
{
    glm::vec2 topLeft;
    float width = 0.0f;
    float height = 0.0f;

    // TODO: why not just a reference?
    std::shared_ptr<Texture> texture;
};

struct AnimationPayload
{
    char animationName[128];
    char animationJsonPath[256];
};

struct FontPayload
{
    char fontPath[256];
};
