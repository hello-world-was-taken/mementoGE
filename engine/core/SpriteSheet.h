#include <memory>

#include "opengl/Texture.h"
#include "core/Sprite.h"

class SpriteSheet
{
private:
    std::string mTexturePath;
    bool mIsTextureAtlas;
    // The size of one sprite in a sprite sheet. Assumed to be a square.
    unsigned int m_subTextureSize = 100;
    unsigned int m_subTextureGap = 0;

    std::shared_ptr<Texture> m_texture;
    std::vector<Sprite> m_sprites;

private:
    void initializeSprites();

public:
    SpriteSheet(
        std::string&& texturePath,
        bool isTextureAtlas,
        unsigned int subTextureSize,
        unsigned int m_subTextureGap);
    ~SpriteSheet();

    unsigned int getSubTextureSize();
    std::vector<Sprite> getSprites();
    std::shared_ptr<Texture> getTexture();
};
