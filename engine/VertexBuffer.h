#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include <vector>
#include "Vertex.h"
#include "VertexAttribute.h"



class VertexBuffer {

private:
    unsigned int m_id;
    std::vector<VertexAttribute> m_vertex_attributes;
public:
    VertexBuffer(std::vector<Vertex> vertices, unsigned int draw_type);
    ~VertexBuffer();
    void bind() const;
    void unbind() const;
    void addAttribute(VertexAttribute vertex_attribute);
    void setAttributePointers() const;
};

#endif // VERTEX_BUFFER_H
