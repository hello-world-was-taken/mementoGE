#pragma once

#include "core/components/Sprite.h"

#include "opengl/Texture.h"

#include <memory>
#include <filesystem>

/**
 * @brief Texture atlas sprite representation
 */
class SpriteSheet
{
public:
    SpriteSheet(std::shared_ptr<Texture> texture);
    ~SpriteSheet();

    static std::shared_ptr<SpriteSheet> fromJson(const std::filesystem::path &jsonPath);

    void addSprite(Sprite &&sprite);
    void updateSpriteSizes();

    std::vector<Sprite> getSprites();
    std::shared_ptr<Texture> getTexture();

public:
    // TODO: update - public for testing purposes
    // TODO: we no longer need these since we started json load
    float m_spriteW;
    float m_spriteH;
    float m_spriteGapX;
    float m_spriteGapY;

private:
    int m_columns;
    int m_spriteCount;

    std::filesystem::path m_jsonPath;
    std::shared_ptr<Texture> m_texture;
    std::vector<Sprite> m_sprites;
};
