#include "core/SpriteSheet.h"
#include "core/Sprite.h"
#include "core/ResourceManager.h"

#include "opengl/Texture.h"

#include "util/PathUtils.h"

#include <memory>
#include <string>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.h>

SpriteSheet::SpriteSheet(std::shared_ptr<Texture> texture) : m_texture{texture}
{
}

std::shared_ptr<SpriteSheet> SpriteSheet::fromJson(const std::filesystem::path &jsonPath)
{
    std::ifstream file(jsonPath);
    nlohmann::json data;
    file >> data;

    std::filesystem::path texturePath = getTexturePathFromJson(jsonPath);
    std::shared_ptr<Texture> tex = ResourceManager::instance().getTexture(texturePath, true);
    std::shared_ptr<SpriteSheet> spriteSheet = std::make_shared<SpriteSheet>(tex);

    float spriteW = data["spriteWidth"];
    float spriteH = data["spriteHeight"];
    float spriteGapX = data["spriteGapX"];
    float spriteGapY = data["spriteGapY"];

    int columns = data["columns"];
    int spriteCount = data["spriteCount"];

    for (int i = 0; i < spriteCount; i++)
    {

        int col = i % columns;
        int row = i / columns;

        Sprite s{};
        float x = col * (spriteW + spriteGapX);
        float y = row * (spriteH + spriteGapY);
        float w = spriteW;
        float h = spriteH;

        spriteSheet->addSprite(Sprite(texturePath, glm::vec2{x, y}, w, h));
    }

    return spriteSheet;
}

SpriteSheet::~SpriteSheet()
{
}

void SpriteSheet::addSprite(Sprite &&sprite)
{
    m_sprites.push_back(sprite);
}

std::vector<Sprite> SpriteSheet::getSprites()
{
    return m_sprites;
}

std::shared_ptr<Texture> SpriteSheet::getTexture()
{
    return m_texture;
}