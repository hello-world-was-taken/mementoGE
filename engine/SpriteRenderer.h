#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <vector>

#include "Texture.h"

class SpriteRenderer
{
private:
    const char *tag = "SpriteRenderer";
    std::vector<glm::vec2> textureCoordinates; // 4 vertices, 2D
    glm::vec4 color;
    std::shared_ptr<Texture> texture;
    unsigned int m_spriteHeight = 0;
    unsigned int m_spriteWidth = 0;
    unsigned int m_xIndex = 0;
    unsigned int m_yIndex = 0;

public:
    SpriteRenderer(glm::vec4 color, unsigned int spriteWidth = 0, unsigned int spriteHeight = 0, unsigned int xIndex = 0, unsigned int yIndex = 0);
    SpriteRenderer(std::shared_ptr<Texture> texture, unsigned int spriteWidth = 0, unsigned int spriteHeight = 0, unsigned int xIndex = 0, unsigned int yIndex = 0);
    ~SpriteRenderer();

    void start();
    void update(float deltaTime);
    void initializeTextureCoordinate();
    std::vector<glm::vec2> getTextureCoordinates();
    std::shared_ptr<Texture> getTexture();
    glm::vec4 getColor();
};