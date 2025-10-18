#pragma once

#include "core/components/Sprite.h"

#include <string>
#include <vector>

struct Frame
{
    Sprite sprite;
    float duration;
};

struct Animation
{
    std::string name;
    std::vector<Frame> frames;
    bool loop = true;
};
