#pragma once

#include <iostream>
#include <vector>
#include <GL/glew.h>

/*
Vertex - A vertex is the combination of all the vertex attributes that belong together.
Vertex Attribute - The different properties contained with in a vertex like position,
color, texture and so on.

Vertex Attributes tell the GPU how to interpret the vertex data we passed.
EX. From 0-3 is position, 3-6 is color, 6-8 is texture and so on.
*/
struct VertexAttribute
{
    unsigned int m_vector_length; // Number of elements that make up the attribute
    unsigned int m_type;          // Data type of the attribute
    unsigned char m_normalized;   // Whether the data should be normalized
    unsigned int m_stride;        // The number of bytes between each vertex
    const void *m_offset;         // The offset of the attribute in the vertex
};