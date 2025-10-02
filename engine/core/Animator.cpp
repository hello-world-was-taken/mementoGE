#include "core/Animator.h"

#include <memory>
#include <string>

Animator::Animator(std::shared_ptr<AnimationMap> animMap, const std::string &defaultAnim)
    : animationMap{animMap},
      currentAnimation{defaultAnim},
      animationPlayer{animationMap->getAnimation(defaultAnim)}
{
}

void Animator::play(const std::string &name, bool loop)
{
    currentAnimation = name;
    animationPlayer.play(animationMap->getAnimation(name), loop);
}

void Animator::update()
{
    animationPlayer.update();
}

const Sprite &Animator::getCurrentSprite() const
{
    return animationPlayer.getCurrentFrame().sprite;
}

void Animator::serialize(YAML::Emitter &out) const
{
    out << YAML::Key << "Animator";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "AnimationMapJsonPath" << YAML::Value << animationMap->getJsonPath();
    out << YAML::Key << "CurrentAnimation" << YAML::Value << currentAnimation;

    out << YAML::EndMap;
}

void Animator::deserialize(const YAML::Node &node)
{
    if (!node["Animator"])
        return;

    auto animPath = node["Animator"]["AnimationMapJsonPath"].as<std::string>();
    animationMap = AnimationMap::fromJson(animPath);

    currentAnimation = node["Animator"]["CurrentAnimation"].as<std::string>();
    animationPlayer.play(animationMap->getAnimation(currentAnimation), true);
}