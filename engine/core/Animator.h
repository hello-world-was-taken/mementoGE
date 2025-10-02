#pragma once

#include "core/AnimationPlayer.h"
#include "core/AnimationMap.h"
#include "core/Sprite.h"

#include <memory>
#include <string>

class Animator
{
public:
    Animator() = default;
    Animator(std::shared_ptr<AnimationMap> animMap, const std::string &defaultAnim);

    void play(const std::string &name, bool loop = true);
    void update();
    const Sprite &getCurrentSprite() const;

    void serialize(YAML::Emitter &out) const;
    void deserialize(const YAML::Node &node);

// private:
    // TODO: making them public to display on imgui
    std::shared_ptr<AnimationMap> animationMap;
    std::string currentAnimation;
    AnimationPlayer animationPlayer;
};
