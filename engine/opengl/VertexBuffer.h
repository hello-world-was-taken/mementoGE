#pragma once

#include <vector>

#include "engine/opengl/Vertex.h"
#include "engine/opengl/VertexAttribute.h"

class VertexBuffer
{
public:
    VertexBuffer() = default;
    VertexBuffer(unsigned int buffer_size, unsigned int draw_type);
    ~VertexBuffer();
    void bind() const;
    void unbind() const;
    void addAttribute(VertexAttribute vertex_attribute);
    void setAttributePointers() const;
    void updateBufferData(std::vector<Vertex> vertices);

private:
    unsigned int m_id;
    std::vector<VertexAttribute> m_vertex_attributes;
};
