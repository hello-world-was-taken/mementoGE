#pragma once

#include "core/Sprite.h"

#include <string>
#include <vector>

struct Frame
{
    // TODO: might be worth to re-use Sprites using shared_ptr
    Sprite sprite;
    float duration;
};

struct Animation
{
    std::string name;
    std::vector<Frame> frames;
    bool loop = true;
};
