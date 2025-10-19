#include "core/components/Sprite.h"

#include "core/SpriteSheet.h"
#include "core/GlResourceManager.h"

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
    std::shared_ptr<Texture> tex = GlResourceManager::instance().getTexture(texturePath, true);
    std::shared_ptr<SpriteSheet> spriteSheet = std::make_shared<SpriteSheet>(tex);
    spriteSheet->m_jsonPath = jsonPath;

    for (auto &[frameName, frameInfo] : data["frames"].items())
    {
        spriteSheet->addSprite(
            {
                {frameInfo["x"].get<float>(), frameInfo["y"].get<float>()},
                frameInfo["w"].get<float>(),
                frameInfo["h"].get<float>(),
                tex
            });
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
        m_sprites.push_back(Sprite{glm::vec2{x, y}, w, h, m_texture});
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