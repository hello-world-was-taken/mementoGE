#pragma once

#include "opengl/VertexAttribute.h"

class VertexArray
{
public:
    VertexArray();
    ~VertexArray();
    void bind() const;
    void unbind() const;
    void attachVertexAttribute(VertexAttribute vertex_attribute);

private:
    void applyAttributePointers() const;

private:
    unsigned int m_id;
    std::vector<VertexAttribute> m_vertex_attributes;
};