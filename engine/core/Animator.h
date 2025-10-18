#pragma once

#include "core/components/Sprite.h"

#include "core/AnimationPlayer.h"
#include "core/AnimationMap.h"

#include <memory>
#include <string>

class Animator
{
public:
    Animator() = default;

    void play(const std::string &name, bool loop = true);
    void update();
    const Sprite &getCurrentSprite() const;

    void serialize(YAML::Emitter &out) const;
    void deserialize(const YAML::Node &node);

// private:
    // TODO: making them public to display on imgui
    std::map<std::string, std::string> animationSourceMap; // "walk" -> "assets/character.json"
    std::string currentAnimation;
    AnimationPlayer animationPlayer;
};
