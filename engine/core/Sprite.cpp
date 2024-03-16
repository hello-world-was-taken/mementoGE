#include "engine/core/Sprite.h"

Sprite::Sprite(
    std::shared_ptr<Texture> texture)
{
    m_texture = texture;
}

// subTextureSpanX: The span of the subtexture in the x direction.
// subTextureSpanY: The span of the subtexture in the y direction.
// subTextureSize: The size of the subtexture. Assumed to be a square.
// subTextureIndexX: The index of the subtexture in the x direction where the origin is the bottom left corner.
// subTextureIndexY: The index of the subtexture in the y direction where the origin is the bottom left corner.
Sprite::Sprite(
    std::shared_ptr<Texture> texture,
    unsigned int subTextureSpanX,
    unsigned int subTextureSpanY,
    unsigned int subTextureSize,
    unsigned int subTextureIndexX,
    unsigned int subTextureIndexY)
{
    m_texture = texture;

    if (texture->isTextureAtlas())
    {
        glm::vec2 topLeft = {float(subTextureSize * subTextureIndexX) / float(texture->getWidth()),
                             float((subTextureSize * (subTextureIndexY + subTextureSpanY))) / float(texture->getHeight())};

        glm::vec2 bottomLeft = {float(subTextureSize * subTextureIndexX) / float(texture->getWidth()),
                                float(subTextureSize * subTextureIndexY) / float(texture->getHeight())};

        glm::vec2 bottomRight = {float(subTextureSize * (subTextureIndexX + subTextureSpanX)) / float(texture->getWidth()),
                                 float(subTextureSize * (subTextureIndexY)) / float(texture->getHeight())};

        glm::vec2 topRight = {float(subTextureSize * (subTextureIndexX + subTextureSpanX)) / float(texture->getWidth()),
                              float(subTextureSize * (subTextureIndexY + subTextureSpanY)) / float(texture->getHeight())};

        m_textureCoordinates = {
            topLeft,
            bottomLeft,
            bottomRight,
            topRight,
        };
    }
}

Sprite::~Sprite()
{
}

std::vector<glm::vec2> Sprite::getTextureCoordinates()
{
    return m_textureCoordinates;
}

std::shared_ptr<Texture> Sprite::getTexture()
{
    return m_texture;
}

glm::vec4 Sprite::getColor()
{
    return color;
}