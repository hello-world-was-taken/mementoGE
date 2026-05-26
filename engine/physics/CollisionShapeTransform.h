#pragma once

#include <glm/glm.hpp>

inline glm::vec2 mirrorCollisionOffset(glm::vec2 offset, bool flipX, bool flipY)
{
    if (flipX)
        offset.x = -offset.x;
    if (flipY)
        offset.y = -offset.y;
    return offset;
}

inline float mirrorCollisionRotation(float rotationDegrees, bool flipX, bool flipY)
{
    return flipX != flipY ? -rotationDegrees : rotationDegrees;
}
