#include "core/Animator.h"
#include "core/AssetManager.h"

#include "util/PathUtils.h"

#include <iostream>
#include <memory>
#include <string>

void Animator::play(std::string name, bool loop)
{
    auto it = animationSourceMap.find(name);
    if (it == animationSourceMap.end())
    {
        std::cout << "Animation " << name << " not found" << std::endl;
        name = "android";
        animationSourceMap[name] = getFilePath("assets/texture/android.json");
    }

    std::string jsonPath = animationSourceMap[name];
    auto animMap = AssetManager::instance().getAnimationMap(jsonPath);

    currentAnimation = name;
    animationPlayer.play(animMap->getAnimation(name), loop);
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

    out << YAML::Key << "CurrentAnimation" << YAML::Value << currentAnimation;

    out << YAML::Key << "Animations";
    out << YAML::Value << YAML::BeginMap;
    for (auto &[animationName, animationPath] : animationSourceMap)
    {
        out << YAML::Key << animationName << YAML::Value << animationPath;
    }
    out << YAML::EndMap;

    out << YAML::EndMap;
}

void Animator::deserialize(const YAML::Node &node)
{
    if (!node["Animator"])
        return;

    auto animationsMap = node["Animator"]["Animations"];
    for (YAML::const_iterator it = animationsMap.begin(); it != animationsMap.end(); it++)
    {
        auto animationName = it->first.as<std::string>();
        auto animationJsonPath = it->second.as<std::string>();

        animationSourceMap[animationName] = animationJsonPath;
    }

    currentAnimation = node["Animator"]["CurrentAnimation"].as<std::string>();
    if (!currentAnimation.empty())
    {
        play(currentAnimation, true);
    }
}