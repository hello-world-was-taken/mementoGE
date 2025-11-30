#include "core/components/Sprite.h"

#include "core/GlResourceManager.h"
#include "core/SpriteSheet.h"

#include "opengl/Texture.h"

#include "util/PathUtils.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.h>
#include <string>

SpriteSheet::SpriteSheet(std::shared_ptr<Texture> texture) : m_texture{texture}
{
    fullSprite = Sprite{{0.0f, 0.0f}, (float)m_texture->getWidth(), (float)m_texture->getHeight(), m_texture};
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
        spriteSheet->addSprite({{frameInfo["x"].get<float>(), frameInfo["y"].get<float>()}, frameInfo["w"].get<float>(),
            frameInfo["h"].get<float>(), tex});
    }

    return spriteSheet;
}

SpriteSheet::~SpriteSheet()
{
}

void SpriteSheet::addSprite(Sprite &&sprite)
{
    // TODO: emplace_back?
    m_sprites.push_back(sprite);
}

std::vector<Sprite>& SpriteSheet::getSprites()
{
    return m_sprites;
}

std::shared_ptr<Texture> SpriteSheet::getTexture()
{
    return m_texture;
}