#include "SpriteRenderer.h"

SpriteRenderer::SpriteRenderer(glm::vec4 color)
{
    this->color = color;

    initializeTextureCoordinate();
}

SpriteRenderer::SpriteRenderer(std::shared_ptr<Texture> texture, unsigned int spriteWidth, unsigned int spriteHeight, unsigned int xIndex, unsigned int yIndex)
{
    this->texture = texture;
    this->m_spriteWidth = spriteWidth;
    this->m_spriteHeight = spriteHeight;
    this->m_xIndex = xIndex;
    this->m_yIndex = yIndex;

    initializeTextureCoordinate();
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::initializeTextureCoordinate()
{
    if (texture->isTextureAtlas())
    {
        this->textureCoordinates.push_back({float(m_spriteWidth * m_xIndex) / float(this->texture->getWidth()),
                                            1 - float((m_spriteHeight * m_yIndex)) / float(this->texture->getHeight())}); // top left
        this->textureCoordinates.push_back({float(m_spriteWidth * m_xIndex) / float(this->texture->getWidth()),
                                            1 - float(m_spriteHeight * (m_yIndex + 1)) / float(this->texture->getHeight())}); // bottom left
        this->textureCoordinates.push_back({float(m_spriteWidth * (m_xIndex + 1)) / float(this->texture->getWidth()),
                                            1 - float(m_spriteHeight * (m_yIndex + 1)) / float(this->texture->getHeight())}); // bottom right
        this->textureCoordinates.push_back({float(m_spriteWidth * (m_xIndex + 1)) / float(this->texture->getWidth()),
                                            1 - float(m_spriteHeight * m_yIndex) / float(this->texture->getHeight())}); // top right
    }
    else
    {
        this->textureCoordinates.push_back({0.0f, 1.0f}); // top left
        this->textureCoordinates.push_back({0.0f, 0.0f}); // bottom left
        this->textureCoordinates.push_back({1.0f, 0.0f}); // bottom right
        this->textureCoordinates.push_back({1.0f, 1.0f}); // top right
    }

    for (int i = 0; i < this->textureCoordinates.size(); i++)
    {
        std::cout << " - " << this->textureCoordinates[i].x << ", " << this->textureCoordinates[i].y << std::endl;
    }
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
    if (this->texture == nullptr)
    {
        std::cout << "SpriteRenderer::getTextureCoordinates() - texture is null" << std::endl;
    }
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

unsigned int SpriteRenderer::getSpriteWidth()
{
    return this->m_spriteWidth;
}

unsigned int SpriteRenderer::getSpriteHeight()
{
    return this->m_spriteHeight;
}

unsigned int SpriteRenderer::getXIndex()
{
    return this->m_xIndex;
}

unsigned int SpriteRenderer::getYIndex()
{
    return this->m_yIndex;
}