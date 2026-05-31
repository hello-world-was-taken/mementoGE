#include "core/AssetManager.h"
#include "core/AnimationMap.h"
#include "core/GlResourceManager.h"
#include "core/SpriteSheet.h"
#include "util/LogChannels.h"

#include <memory>
#include <sawyer/Logger.h>
#include <string>

AssetManager &AssetManager::instance()
{
    static AssetManager am;
    return am;
}

std::shared_ptr<SpriteSheet> AssetManager::getSpriteSheet(const std::string &jsonPath)
{
    auto it = spriteSheetByPath.find(jsonPath);
    if (it != spriteSheetByPath.end())
    {
        return it->second;
    }

    std::shared_ptr<SpriteSheet> spriteSheet;
    try
    {
        spriteSheet = SpriteSheet::fromJson(jsonPath);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(LogChannel::Asset, "Failed to load sprite sheet '{}': {}", jsonPath, e.what());
    }

    spriteSheetByPath[jsonPath] = spriteSheet;
    return spriteSheet;
}

std::shared_ptr<AnimationMap> AssetManager::getAnimationMap(const std::string &jsonPath)
{
    auto it = animationMaps.find(jsonPath);
    if (it != animationMaps.end())
    {
        return it->second;
    }

    std::shared_ptr<AnimationMap> animMap;
    try
    {
        animMap = AnimationMap::fromJson(jsonPath);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(LogChannel::Asset, "Failed to load animation map '{}': {}", jsonPath, e.what());
    }

    animationMaps[jsonPath] = animMap;
    return animationMaps[jsonPath];
}

std::shared_ptr<Font> AssetManager::getFont(const std::string &fontPath)
{
    auto it = fontByFontPath.find(fontPath);
    if (it != fontByFontPath.end())
    {
        return it->second;
    }
    auto fontAtlas = GlResourceManager::instance().getFontTexture(fontPath);
    auto packedChar = GlResourceManager::instance().getFontPackedChar(fontPath);
    auto alignedQuad = GlResourceManager::instance().getFontAlignedQuad(fontPath);
    auto metrics = GlResourceManager::instance().getFontMetrics(fontPath);
    auto font = std::make_shared<Font>(Font{
        fontPath,
        fontAtlas,
        packedChar,
        alignedQuad,
        64.0f,
        32,
        static_cast<unsigned int>(Font::glyphCount),
        metrics}); // FIXME: copies atlas data.

    fontByFontPath[fontPath] = font;

    return font;
}

unsigned int AssetManager::getAudioClip(const std::string &audioPath)
{
    auto it = audioByPath.find(audioPath);
    if (it != audioByPath.end())
    {
        return it->second;
    }

    unsigned int buffer = AudioResourceManager::instance().getAudioBuffer(audioPath);

    audioByPath[audioPath] = buffer;
    return buffer;
}

void AssetManager::forgetAsset(const std::string &assetPath)
{
    spriteSheetByPath.erase(assetPath);
    animationMaps.erase(assetPath);
    fontByFontPath.erase(assetPath);
    audioByPath.erase(assetPath);
}
