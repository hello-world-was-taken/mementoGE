#pragma once
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texture;
    float texIndex;
};