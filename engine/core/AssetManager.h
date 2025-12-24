#pragma once

#pragma once
#include "core/AnimationMap.h"
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

    // Sprite sheets
    // TODO: shared_ptr vs returning const reference?
    std::shared_ptr<SpriteSheet> getSpriteSheet(const std::string &jsonPath);

    // Animation map
    std::shared_ptr<AnimationMap> getAnimationMap(const std::string &jsonPath);
    std::shared_ptr<Font> getFont(const std::string &fontPath);

private:
    AssetManager() = default;
    ~AssetManager() = default;

    // no copy/move
    AssetManager(const AssetManager &) = delete;
    AssetManager &operator=(const AssetManager &) = delete;
    AssetManager(AssetManager &&) = delete;
    AssetManager &operator=(AssetManager &&) = delete;

    std::map<std::string, std::shared_ptr<SpriteSheet>> spriteSheetByPath;
    // FIXME: rename AnimationMap (to AnimationPerSpirtesheet or sth) and use nameBySth format for maps
    std::map<std::string, std::shared_ptr<AnimationMap>> animationMaps;
    std::map<std::string, std::shared_ptr<Font>> fontByFontPath;
};
