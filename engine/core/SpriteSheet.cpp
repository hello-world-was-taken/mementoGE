#include <string>

#include "core/SpriteSheet.h"
#include "core/Resource.h"

SpriteSheet::SpriteSheet(
    std::string texturePath,
    bool isTextureAtlas,
    unsigned int subTextureSize,
    unsigned int subTextureGap)
: mTexturePath{texturePath}
, mIsTextureAtlas{isTextureAtlas}
, m_subTextureSize{subTextureSize}
, m_subTextureGap{subTextureGap}
{
    m_texture = Resource::getTexture(texturePath, isTextureAtlas);
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
                mTexturePath,
                mIsTextureAtlas,
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

std::shared_ptr<Texture> SpriteSheet::getTexture()
{
    return m_texture;
}