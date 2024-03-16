#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <memory>

#include "engine/opengl/Texture.h"

class Sprite
{
private:
    std::shared_ptr<Texture> m_texture;
    std::vector<glm::vec2> m_textureCoordinates = {
        {0.0f, 1.0f}, // top left
        {0.0f, 0.0f}, // bottom left
        {1.0f, 0.0f}, // bottom right
        {1.0f, 1.0f}, // top right
    };
    
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // white

    // TODO: a sprite could just be a color as well
public:
    Sprite(
        std::shared_ptr<Texture> texture);
    Sprite(
        std::shared_ptr<Texture> texture,
        unsigned int subTextureSpanX,
        unsigned int subTextureSpanY,
        unsigned int subTextureSize,
        unsigned int subTextureIndexX,
        unsigned int subTextureIndexY);
    ~Sprite();

    std::vector<glm::vec2> getTextureCoordinates();
    std::shared_ptr<Texture> getTexture();
    glm::vec4 getColor();
};
