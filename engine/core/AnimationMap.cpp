#include "core/components/Animation.h"
#include "core/AnimationMap.h"

#include "opengl/Texture.h"

#include "util/PathUtils.h"

#include <memory>
#include <fstream>
#include <nlohmann/json.h>

AnimationMap::AnimationMap(std::shared_ptr<Texture> texture)
    : m_texture(texture) {}

std::shared_ptr<AnimationMap> AnimationMap::fromJson(const std::filesystem::path &jsonPath)
{
    std::ifstream file(jsonPath);
    nlohmann::json data;
    file >> data;

    std::filesystem::path texturePath = getTexturePathFromJson(jsonPath);
    auto tex = GlResourceManager::instance().getTexture(texturePath, true);
    auto animMap = std::make_shared<AnimationMap>(tex);
    animMap->setJsonPath(jsonPath);

    // parse the frames
    std::unordered_map<std::string, Frame> frameLookup;
    for (auto &[frameName, frameInfo] : data["frames"].items())
    {
        Frame f;
        f.sprite = Sprite{
            {frameInfo["x"].get<float>(), frameInfo["y"].get<float>()},
            frameInfo["w"].get<float>(),
            frameInfo["h"].get<float>(),
            tex};

        f.duration = 0.1; // todo: should we make this part of the frame in our metadata?
        frameLookup[frameName] = f;
    }

    // build animations
    for (auto &[animName, animInfo] : data["animations"].items())
    {
        Animation anim;
        anim.name = animName;

        // Each animation lists frame *keys* (e.g. "run01", "run02", etc.)
        for (auto &frameEntry : animInfo["frames"])
        {
            std::string frameKey;
            frameKey = frameEntry.get<std::string>();

            auto it = frameLookup.find(frameKey);
            if (it == frameLookup.end())
                continue;

            const Frame &frame = it->second;
            anim.frames.push_back(frame);
        }

        animMap->addAnimation(anim);
    }

    return animMap;
}

void AnimationMap::addAnimation(const Animation &anim)
{
    m_animations[anim.name] = anim;
}

const Animation &AnimationMap::getAnimation(const std::string &name) const
{
    auto it = m_animations.find(name);
    if (it != m_animations.end())
    {
        return it->second;
    }

    throw std::runtime_error("Animation not found");
}

std::map<std::string, Animation> &AnimationMap::getAnimations()
{
    return m_animations;
}

void AnimationMap::setJsonPath(const std::filesystem::path &jsonPath)
{
    m_jsonPath = jsonPath;
}

std::filesystem::path AnimationMap::getJsonPath()
{
    return m_jsonPath;
}

std::shared_ptr<Texture> AnimationMap::getTexture() const
{
    return m_texture;
}