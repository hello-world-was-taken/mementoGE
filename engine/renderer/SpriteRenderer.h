#pragma once

#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#include "engine/opengl/Texture.h"

class SpriteRenderer
{
private:
    const char *tag = "SpriteRenderer";
    std::vector<glm::vec2> textureCoordinates;  // 4 vertices, 2D
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // white
    std::shared_ptr<Texture> texture = nullptr;

    // By default, we assume the subTextureSize is enough to cover our current sprite.
    // So, m_spriteWidth and m_spriteHeight are set to 1. For the cases, where our current
    // subTexture spans more than that, we can set these values accordingly.
    unsigned int m_subTextureSpanX = 1;
    unsigned int m_subTextureSpanY = 1;

    // The size of one sprite in a sprite sheet. Assumed to be a square.
    unsigned int m_subTextureSize = 100;

    // The index are counted by taking bottom left corner of the spritesheet as the origin.
    unsigned int m_subTextureIndexX = 0;
    unsigned int m_subTextureIndexY = 0;

public:
    SpriteRenderer(glm::vec4 color);
    SpriteRenderer(
        std::shared_ptr<Texture> texture,
        unsigned int subTextureSpanX = 1,
        unsigned int subTextureSpanY = 1,
        unsigned int subTextureSize = 100,
        unsigned int subTextureIndexX = 0,
        unsigned int subTextureIndexY = 0);

        ~SpriteRenderer();

    void start();
    void update(float deltaTime);
    void initializeTextureCoordinate();
    std::vector<glm::vec2> getTextureCoordinates();
    std::shared_ptr<Texture> getTexture();
    glm::vec4 getColor();
    unsigned int getSubTextureSpanX();
    unsigned int getSubTextureSpanY();
    unsigned int getSubTextureSize();
    unsigned int getSubTextureIndexX();
    unsigned int getSubTextureIndexY();
};