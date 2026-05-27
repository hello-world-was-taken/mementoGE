#include "core/AnimationMap.h"
#include "core/components/Animation.h"

#include "opengl/Texture.h"

#include "util/LogChannels.h"
#include "util/PathUtils.h"

#include <fstream>
#include <memory>
#include <nlohmann/json.h>
#include <sawyer/Logger.h>

AnimationMap::AnimationMap(std::shared_ptr<Texture> texture) : m_texture(texture)
{
}

std::shared_ptr<AnimationMap> AnimationMap::fromJson(const std::filesystem::path &jsonPath)
{
    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        LOG_ERROR(LogChannel::Asset, "Could not open animation file '{}'", jsonPath.string());
        return nullptr;
    }

    nlohmann::json data;
    try
    {
        file >> data;
    }
    catch (const nlohmann::json::exception &e)
    {
        LOG_ERROR(LogChannel::Asset, "Could not parse animation file '{}': {}", jsonPath.string(), e.what());
        return nullptr;
    }

    try
    {
        std::filesystem::path texturePath = getTexturePathFromJson(data);
        auto tex = GlResourceManager::instance().getTexture(texturePath);
        auto animMap = std::make_shared<AnimationMap>(tex);
        animMap->setJsonPath(jsonPath);

        // parse the frames
        std::unordered_map<std::string, Frame> frameLookup;
        for (auto &[frameName, frameInfo] : data["frames"].items())
        {
            Frame f;
            f.sprite = Sprite{{frameInfo["x"].get<float>(), frameInfo["y"].get<float>()},
                frameInfo["w"].get<float>(),
                frameInfo["h"].get<float>(),
                tex};

            f.duration = frameInfo.value("duration", 0.1f);

            if (frameInfo.contains("sensors"))
            {
                for (auto &[shapeName, shapeInfo] : frameInfo["sensors"].items())
                {
                    SensorShapePose pose;
                    pose.name = shapeName;
                    if (shapeInfo.contains("size"))
                    {
                        pose.size = glm::vec2{shapeInfo["size"][0].get<float>(), shapeInfo["size"][1].get<float>()};
                    }
                    if (shapeInfo.contains("offset"))
                    {
                        pose.offset =
                            glm::vec2{shapeInfo["offset"][0].get<float>(), shapeInfo["offset"][1].get<float>()};
                    }
                    if (shapeInfo.contains("active"))
                    {
                        pose.active = shapeInfo["active"].get<bool>();
                    }
                    f.sensorShapes.push_back(std::move(pose));
                }
            }

            if (frameInfo.contains("colliders"))
            {
                for (auto &[shapeName, shapeInfo] : frameInfo["colliders"].items())
                {
                    BoxColliderShapePose pose;
                    pose.name = shapeName;
                    if (shapeInfo.contains("size"))
                    {
                        pose.size = glm::vec2{shapeInfo["size"][0].get<float>(), shapeInfo["size"][1].get<float>()};
                    }
                    if (shapeInfo.contains("offset"))
                    {
                        pose.offset =
                            glm::vec2{shapeInfo["offset"][0].get<float>(), shapeInfo["offset"][1].get<float>()};
                    }
                    if (shapeInfo.contains("rotation"))
                    {
                        pose.rotation = shapeInfo["rotation"].get<float>();
                    }
                    if (shapeInfo.contains("active"))
                    {
                        pose.active = shapeInfo["active"].get<bool>();
                    }
                    f.colliderShapes.push_back(std::move(pose));
                }
            }
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
                {
                    continue;
                }

                const Frame &frame = it->second;
                anim.frames.push_back(frame);
            }

            animMap->addAnimation(anim);
        }

        return animMap;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(LogChannel::Asset, "Invalid animation file '{}': {}", jsonPath.string(), e.what());
        return nullptr;
    }
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

    throw std::runtime_error("Animation not found: " + name);
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
