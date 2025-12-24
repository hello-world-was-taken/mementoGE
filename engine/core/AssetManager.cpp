#include "core/AssetManager.h"
#include "core/AnimationMap.h"
#include "core/GlResourceManager.h"
#include "core/SpriteSheet.h"

#include <memory>
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

    std::shared_ptr<SpriteSheet> spriteSheet = SpriteSheet::fromJson(jsonPath);

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

    std::shared_ptr<AnimationMap> animMap = AnimationMap::fromJson(jsonPath);

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
    auto font = std::make_shared<Font>(
        Font{fontPath, fontAtlas, packedChar, alignedQuad}); // FIXME: we are copying packedChar here.

    fontByFontPath[fontPath] = font;

    return font;
}