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
    spriteSheet->m_jsonPath = jsonPath;

    spriteSheet->m_spriteW = data["spriteWidth"];
    spriteSheet->m_spriteH = data["spriteHeight"];
    spriteSheet->m_spriteGapX = data["spriteGapX"];
    spriteSheet->m_spriteGapY = data["spriteGapY"];

    spriteSheet->m_columns = data["columns"];
    spriteSheet->m_spriteCount = data["spriteCount"];

    for (int i = 0; i < spriteSheet->m_spriteCount; i++)
    {

        int col = i % spriteSheet->m_columns;
        int row = i / spriteSheet->m_columns;

        Sprite s{};
        float x = col * (spriteSheet->m_spriteW + spriteSheet->m_spriteGapX);
        float y = row * (spriteSheet->m_spriteH + spriteSheet->m_spriteGapY);

        spriteSheet->addSprite(Sprite(
            texturePath,
            glm::vec2{x, y},
            spriteSheet->m_spriteW,
            spriteSheet->m_spriteH));
    }

    return spriteSheet;
}

void SpriteSheet::updateSpriteSizes()
{
    m_sprites.clear();

    for (int i = 0; i < m_spriteCount; i++)
    {

        int col = i % m_columns;
        int row = i / m_columns;

        Sprite s{};
        float x = col * (m_spriteW + m_spriteGapX);
        float y = row * (m_spriteH + m_spriteGapY);
        float w = m_spriteW;
        float h = m_spriteH;

        // TODO: emplace_back?
        m_sprites.push_back(Sprite(m_texture->getFilePath(), glm::vec2{x, y}, w, h));
    }
}

SpriteSheet::~SpriteSheet()
{
}

void SpriteSheet::addSprite(Sprite &&sprite)
{
    // TODO: emplace_back?
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