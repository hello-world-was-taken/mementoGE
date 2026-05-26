#pragma once

#include "core/components/Sprite.h"

#include <optional>
#include <string>
#include <vector>

struct SensorShapePose
{
    std::string name;
    std::optional<glm::vec2> size;
    std::optional<glm::vec2> offset;
    std::optional<bool> active;
};

struct BoxColliderShapePose
{
    std::string name;
    std::optional<glm::vec2> size;
    std::optional<glm::vec2> offset;
    std::optional<float> rotation;
    std::optional<bool> active;
};

struct Frame
{
    Sprite sprite;
    float duration;
    std::vector<SensorShapePose> sensorShapes;
    std::vector<BoxColliderShapePose> colliderShapes;
};

struct Animation
{
    std::string name;
    std::vector<Frame> frames;
    bool loop = true;
};
