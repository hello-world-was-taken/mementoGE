#pragma once

#include "core/AnimationMap.h"
#include "core/AudioResourceManager.h"
#include "core/Font.h"
#include "core/SpriteSheet.h"

#include <map>
#include <memory>
#include <string>

class SpriteSheet;

class AssetManager
{
public:
    static AssetManager &instance();

    // TODO: shared_ptr vs returning const reference for all resource types
    std::shared_ptr<SpriteSheet> getSpriteSheet(const std::string &jsonPath);

    std::shared_ptr<AnimationMap> getAnimationMap(const std::string &jsonPath);
    std::shared_ptr<Font> getFont(const std::string &fontPath);
    unsigned int getAudioClip(const std::string &audioPath);

private:
    AssetManager() = default;
    ~AssetManager() = default;

    // no copy/move
    AssetManager(const AssetManager &) = delete;
    AssetManager &operator=(const AssetManager &) = delete;
    AssetManager(AssetManager &&) = delete;
    AssetManager &operator=(AssetManager &&) = delete;

    // FIXME: use unordered_map unless map is required.
    // Also, change all the names of map varialbes to namebysth format
    std::map<std::string, std::shared_ptr<SpriteSheet>> spriteSheetByPath;
    // FIXME: rename AnimationMap (to AnimationPerSpirtesheet or sth) and use nameBySth format for maps
    std::map<std::string, std::shared_ptr<AnimationMap>> animationMaps;
    std::map<std::string, std::shared_ptr<Font>> fontByFontPath;
    std::map<std::string, unsigned int> audioByPath;
};
