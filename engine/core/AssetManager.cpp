#include "core/AssetManager.h"
#include "core/AnimationMap.h"
#include "core/SpriteSheet.h"

#include <memory>
#include <string>

AssetManager& AssetManager::instance()
{
    static AssetManager am;
    return am;
}

std::shared_ptr<SpriteSheet> AssetManager::getSpriteSheet(const std::string &jsonPath){
    auto it = spriteSheets.find(jsonPath);
    if (it != spriteSheets.end())
    {
        return it->second;
    }

    std::shared_ptr<SpriteSheet> spriteSheet = SpriteSheet::fromJson(jsonPath);

    spriteSheets[jsonPath] = spriteSheet;
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