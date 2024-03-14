#include "SpriteRenderer.h"

SpriteRenderer::SpriteRenderer(glm::vec4 color)
{
    this->color = color;

    initializeTextureCoordinate();
}

SpriteRenderer::SpriteRenderer(
    std::shared_ptr<Texture> texture,
    unsigned int subTextureSpanX,
    unsigned int subTextureSpanY,
    unsigned int subTextureSize,
    unsigned int subTextureIndexX,
    unsigned int subTextureIndexY)
{
    this->texture = texture;
    this->m_subTextureSpanX = subTextureSpanX;
    this->m_subTextureSpanY = subTextureSpanY;
    this->m_subTextureSize = subTextureSize;
    this->m_subTextureIndexX = subTextureIndexX;
    this->m_subTextureIndexY = subTextureIndexY;

    initializeTextureCoordinate();
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::initializeTextureCoordinate()
{
    if (texture->isTextureAtlas())
    {
        this->textureCoordinates
            .push_back({float(m_subTextureSize * m_subTextureIndexX) / float(this->texture->getWidth()),
                        float((m_subTextureSize * (m_subTextureIndexY + m_subTextureSpanY))) / float(this->texture->getHeight())}); // top left
        this->textureCoordinates
            .push_back({float(m_subTextureSize * m_subTextureIndexX) / float(this->texture->getWidth()),
                        float(m_subTextureSize * m_subTextureIndexY) / float(this->texture->getHeight())}); // bottom left
        this->textureCoordinates
            .push_back({float(m_subTextureSize * (m_subTextureIndexX + m_subTextureSpanX)) / float(this->texture->getWidth()),
                        float(m_subTextureSize * (m_subTextureIndexY)) / float(this->texture->getHeight())}); // bottom right
        this->textureCoordinates
            .push_back({float(m_subTextureSize * (m_subTextureIndexX + m_subTextureSpanX)) / float(this->texture->getWidth()),
                        float(m_subTextureSize * (m_subTextureIndexY + m_subTextureSpanY)) / float(this->texture->getHeight())}); // top right
    }
    else
    {
        this->textureCoordinates.push_back({0.0f, 1.0f}); // top left
        this->textureCoordinates.push_back({0.0f, 0.0f}); // bottom left
        this->textureCoordinates.push_back({1.0f, 0.0f}); // bottom right
        this->textureCoordinates.push_back({1.0f, 1.0f}); // top right
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

unsigned int SpriteRenderer::getSubTextureSpanX()
{
    return this->m_subTextureSpanX;
}

unsigned int SpriteRenderer::getSubTextureSpanY()
{
    return this->m_subTextureSpanY;
}

unsigned int SpriteRenderer::getSubTextureSize()
{
    return this->m_subTextureSize;
}

unsigned int SpriteRenderer::getSubTextureIndexX()
{
    return this->m_subTextureIndexX;
}

unsigned int SpriteRenderer::getSubTextureIndexY()
{
    return this->m_subTextureIndexY;
}