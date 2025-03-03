#pragma once

#include <vector>

#include "opengl/Vertex.h"
#include "opengl/VertexAttribute.h"

class VertexBuffer
{
public:
    VertexBuffer() = default;
    VertexBuffer(unsigned int buffer_size, unsigned int draw_type);
    ~VertexBuffer();
    void bind() const;
    void unbind() const;
    void updateBufferData(std::vector<Vertex> vertices);

private:
    unsigned int m_id;
    std::vector<VertexAttribute> m_vertex_attributes;
};
