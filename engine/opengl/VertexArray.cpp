#include <GL/glew.h>

#include "engine/opengl/VertexArray.h"

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &this->m_id);
}

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &this->m_id);
}

void VertexArray::bind() const
{
    glBindVertexArray(this->m_id);
    this->applyAttributePointers();
}

void VertexArray::unbind() const
{
    glBindVertexArray(0);
}

void VertexArray::attachVertexAttribute(VertexAttribute vertex_attribute)
{
    this->m_vertex_attributes.push_back(vertex_attribute);
}

void VertexArray::applyAttributePointers() const
{
    for (unsigned int i = 0; i < this->m_vertex_attributes.size(); i++)
    {
        VertexAttribute vertex_attribute = this->m_vertex_attributes[i];
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(
            i,
            vertex_attribute.m_vector_length,
            vertex_attribute.m_type,
            vertex_attribute.m_normalized,
            vertex_attribute.m_stride,
            (void *)vertex_attribute.m_offset);
    }
}