#include "core/components/Sprite.h"

#include "core/GlResourceManager.h"
#include "core/SpriteSheet.h"

#include "opengl/Texture.h"

#include "util/LogChannels.h"
#include "util/PathUtils.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.h>
#include <sawyer/Logger.h>
#include <string>

SpriteSheet::SpriteSheet(std::shared_ptr<Texture> texture) : m_texture{texture}
{
    fullSprite = Sprite{{0.0f, 0.0f}, (float)m_texture->getWidth(), (float)m_texture->getHeight(), m_texture};
}

std::shared_ptr<SpriteSheet> SpriteSheet::fromJson(const std::filesystem::path &jsonPath)
{
    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        LOG_ERROR(LogChannel::Asset, "Could not open sprite sheet file '{}'", jsonPath.string());
        return nullptr;
    }

    nlohmann::json data;
    try
    {
        file >> data;
    }
    catch (const nlohmann::json::exception &e)
    {
        LOG_ERROR(LogChannel::Asset, "Could not parse sprite sheet file '{}': {}", jsonPath.string(), e.what());
        return nullptr;
    }

    try
    {
        std::filesystem::path texturePath = getTexturePathFromJson(data);
        std::shared_ptr<Texture> tex = GlResourceManager::instance().getTexture(texturePath, true);
        std::shared_ptr<SpriteSheet> spriteSheet = std::make_shared<SpriteSheet>(tex);
        spriteSheet->m_jsonPath = jsonPath;

        for (auto &[frameName, frameInfo] : data["frames"].items())
        {
            spriteSheet->addSprite({{frameInfo["x"].get<float>(), frameInfo["y"].get<float>()},
                frameInfo["w"].get<float>(),
                frameInfo["h"].get<float>(),
                tex});
        }

        return spriteSheet;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(LogChannel::Asset, "Invalid sprite sheet file '{}': {}", jsonPath.string(), e.what());
        return nullptr;
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

std::vector<Sprite> &SpriteSheet::getSprites()
{
    return m_sprites;
}

std::shared_ptr<Texture> SpriteSheet::getTexture()
{
    return m_texture;
}
