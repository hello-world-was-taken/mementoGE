#pragma once

#include "core/AssetManager.h"
#include "core/components/Sprite.h"

#include "opengl/Texture.h"

#include <filesystem>
#include <memory>

class AssetManager;

/**
 * @brief Texture atlas sprite representation
 */
class SpriteSheet
{
public:
    SpriteSheet(std::shared_ptr<Texture> texture);
    ~SpriteSheet();

    void addSprite(Sprite &&sprite);

    std::vector<Sprite>& getSprites();
    std::shared_ptr<Texture> getTexture();

    // fromJson should only be called from AssetManager
    friend AssetManager;

private:
    static std::shared_ptr<SpriteSheet> fromJson(const std::filesystem::path &jsonPath);

private:
    std::filesystem::path m_jsonPath;
    std::shared_ptr<Texture> m_texture;
    // TODO: keeping this as a vector may affect the way sprite.getId() works.
    // look into this more
    std::vector<Sprite> m_sprites;

public:
    // sprite representation of the whole spriteSheet.
    // used for editor panels.
    Sprite fullSprite;
};
