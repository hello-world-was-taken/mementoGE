#include <memory>

#include "engine/opengl/Texture.h"
#include "engine/core/Sprite.h"

class SpriteSheet
{
private:
    // The size of one sprite in a sprite sheet. Assumed to be a square.
    unsigned int m_subTextureSize = 100;
    unsigned int m_subTextureGap = 0;

    std::shared_ptr<Texture> m_texture;
    std::vector<Sprite> m_sprites;

public:
    SpriteSheet(
        std::shared_ptr<Texture> texture,
        unsigned int subTextureSize,
        unsigned int m_subTextureGap);
    ~SpriteSheet();

    void initializeSprites();
};
