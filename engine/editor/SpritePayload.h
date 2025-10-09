#pragma once
#include <string>

// TODO: What else can we add here
// Also, can we put it in Sprite.h maybe
struct SpritePayload
{
    int spriteIndex;
};

struct AnimationPayload
{
    char animationName[128];
    char animationJsonPath[256];
};