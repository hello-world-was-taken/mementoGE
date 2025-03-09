#pragma once
#include <glm/glm.hpp>

/**
 * Vertex - A vertex is the combination of all the vertex attributes that belong together.
 * See VertexAttribute.h to see how we describe theses properties to openGL.
 */
struct Vertex
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texture;
    float texIndex;
};