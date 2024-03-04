#pragma once

#include <iostream>
#include <glm/glm.hpp>

#include "Texture.h"

class SpriteRenderer
{
private:
    const char *tag = "SpriteRenderer";
    std::vector<glm::vec2> textureCoordinates;  // 4 vertices, 2D
    glm::vec4 color;
    Texture *texture;

public:
    SpriteRenderer(glm::vec4 color);
    SpriteRenderer(Texture *texture);
    ~SpriteRenderer();

    void start();
    void update(float deltaTime);
    std::vector<glm::vec2> getTextureCoordinates();
    Texture* getTexture();
    glm::vec4 getColor();
};