#pragma once

#include <vector>
#include "Vertex.h"
#include "VertexAttribute.h"



class VertexBuffer {

private:
    unsigned int m_id;
    std::vector<VertexAttribute> m_vertex_attributes;
public:
    VertexBuffer() = default;
    VertexBuffer(unsigned int bufferSize, unsigned int draw_type);
    ~VertexBuffer();
    void bind() const;
    void unbind() const;
    void addAttribute(VertexAttribute vertex_attribute);
    void setAttributePointers() const;
    void updateBufferData(std::vector<Vertex> vertices);
};
