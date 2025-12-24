#pragma once

#include "core/components/Sprite.h"
#include "core/AnimationMap.h"

#include "core/AnimationPlayer.h"

#include <memory>
#include <string>

class Animator
{
public:
    Animator() = default;

    void play(std::string name, bool loop = true);
    void update();
    const Sprite &getCurrentSprite() const;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif

    // private:
    // TODO: making them public to display on imgui
    std::map<std::string, std::string> animationSourceMap; // "walk" -> "assets/character.json"
    std::string currentAnimation;
    AnimationPlayer animationPlayer;
};
