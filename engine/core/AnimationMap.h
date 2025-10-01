#pragma once

#include "core/Animation.h"
#include "core/ResourceManager.h"

#include "opengl/Texture.h"

#include <fstream>
#include <memory>
#include <filesystem>


/**
 * @brief Stores animation per sprite sheet
 */
class AnimationMap
{
public:
    AnimationMap(std::shared_ptr<Texture> texture);

    static std::shared_ptr<AnimationMap> fromJson(const std::filesystem::path &jsonPath);

    void addAnimation(const Animation &anim);
    const Animation &getAnimation(const std::string &name) const;
    std::shared_ptr<Texture> getTexture() const;

private:
    std::map<std::string, Animation> animations;
    std::shared_ptr<Texture> texture;
};
