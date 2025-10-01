#include "core/AnimationMap.h"
#include "core/Animation.h"

#include "opengl/Texture.h"

#include "util/GetExecutableDir.h"

#include <memory>
#include <fstream>
#include <nlohmann/json.h>

AnimationMap::AnimationMap(std::shared_ptr<Texture> texture)
    : texture(texture) {}

std::shared_ptr<AnimationMap> AnimationMap::fromJson(const std::filesystem::path &jsonPath)
{
    std::ifstream file(jsonPath);
    nlohmann::json data;
    file >> data;

    std::filesystem::path texturePath = getFilePath("assets/texture") / data["texture"];
    auto tex = ResourceManager::instance().getTexture(texturePath, true);
    auto set = std::make_shared<AnimationMap>(tex);

    float spriteW = data["spriteWidth"];
    float spriteH = data["spriteHeight"];
    float spriteGapX = data["spriteGapX"];
    float spriteGapY = data["spriteGapY"];

    int columns = data["columns"];

    for (auto &[name, animData] : data["animations"].items())
    {
        Animation anim;
        anim.name = name;
        anim.loop = animData["loop"];

        int startFrame = animData["startFrame"];
        int count = animData["frameCount"];
        float duration = animData["duration"];

        for (int i = 0; i < count; i++)
        {
            int frameIndex = startFrame + i;

            int col = frameIndex % columns;
            int row = frameIndex / columns;

            Frame f;
            // TODO: handle initial frame gaps
            float x = col * (spriteW + spriteGapX);
            float y = row * (spriteH + spriteGapY);

            f.sprite = Sprite{texturePath, {x, y}, spriteW, spriteH};
            f.duration = duration;

            // TODO: emplace_back perhaps
            anim.frames.push_back(f);
        }

        set->addAnimation(anim);
    }

    return set;
}

void AnimationMap::addAnimation(const Animation &anim)
{
    animations[anim.name] = anim;
}

const Animation &AnimationMap::getAnimation(const std::string &name) const
{
    auto it = animations.find(name);
    if (it != animations.end())
    {
        return it->second;
    }

    throw std::runtime_error("Animation not found");
}

std::shared_ptr<Texture> AnimationMap::getTexture() const { return texture; }