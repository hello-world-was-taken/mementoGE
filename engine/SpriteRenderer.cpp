#include "SpriteRenderer.h"

SpriteRenderer::SpriteRenderer(glm::vec4 color)
{
    this->color = color;
    this->texture = nullptr;
    this->textureCoordinates.push_back({0.0f, 1.0f}); // top left
    this->textureCoordinates.push_back({0.0f, 0.0f}); // bottom left
    this->textureCoordinates.push_back({1.0f, 0.0f}); // bottom right
    this->textureCoordinates.push_back({1.0f, 1.0f}); // top right
}

SpriteRenderer::SpriteRenderer(std::shared_ptr<Texture> texture)
{
    this->color = {1.0f, 1.0f, 1.0f, 1.0f}; // white
    this->texture = texture;
    this->textureCoordinates.push_back({0.0f, 1.0f}); // top left
    this->textureCoordinates.push_back({0.0f, 0.0f}); // bottom left
    this->textureCoordinates.push_back({1.0f, 0.0f}); // bottom right
    this->textureCoordinates.push_back({1.0f, 1.0f}); // top right
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::start()
{
    std::cout << "SpriteRenderer::start()" << std::endl;
}

void SpriteRenderer::update(float deltaTime)
{
    std::cout << "SpriteRenderer::update()" << std::endl;
}

std::vector<glm::vec2> SpriteRenderer::getTextureCoordinates()
{
    return this->textureCoordinates;
}

std::shared_ptr<Texture> SpriteRenderer::getTexture()
{
    return this->texture;
}

glm::vec4 SpriteRenderer::getColor()
{
    return this->color;
}