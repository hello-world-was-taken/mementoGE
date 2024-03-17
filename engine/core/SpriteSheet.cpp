#include "engine/core/SpriteSheet.h"

SpriteSheet::SpriteSheet(
    std::shared_ptr<Texture> texture,
    unsigned int subTextureSize,
    unsigned int m_subTextureGap)
{
    m_texture = texture;
    m_subTextureSize = subTextureSize;
    m_subTextureGap = m_subTextureGap;
    m_sprites = std::vector<Sprite>();

    initializeSprites();
}

SpriteSheet::~SpriteSheet()
{
}

// TODO: Add support for when there is a gap between subTextures
void SpriteSheet::initializeSprites()
{
    unsigned int textureWidth = m_texture->getWidth();
    unsigned int textureHeight = m_texture->getHeight();

    unsigned int maxHorizontalSpriteCount = textureWidth / m_subTextureSize;
    unsigned int maxVerticalSpriteCount = textureHeight / m_subTextureSize;

    for (unsigned int y = 0; y < maxVerticalSpriteCount; y++)
    {
        for (unsigned int x = 0; x < maxHorizontalSpriteCount; x++)
        {
            m_sprites.push_back(Sprite(
                m_texture,
                1,
                1,
                m_subTextureSize,
                x,
                y));
        }
    }
}

unsigned int SpriteSheet::getSubTextureSize()
{
    return m_subTextureSize;
}

std::vector<Sprite> SpriteSheet::getSprites()
{
    return m_sprites;
}