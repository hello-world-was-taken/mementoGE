#include <GL/glew.h>

#include "VertexArray.h"
#include "VertexAttribute.h"

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &m_id);
}

void VertexArray::bind() const
{
    glBindVertexArray(m_id);
    setAttributePointers();
}

void VertexArray::unbind() const
{
    glBindVertexArray(0);
}

void VertexArray::attachVertexAttribute(VertexAttribute vertex_attribute)
{
    m_vertex_attributes.push_back(vertex_attribute);
}

void VertexArray::setAttributePointers() const
{
    for (unsigned int i = 0; i < m_vertex_attributes.size(); i++)
    {
        VertexAttribute vertex_attribute = m_vertex_attributes[i];

        // Enabling required because by default they are disabled.
        // Think of layout(location = 0) in vec3 position; and so on
        // in the shaders.
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
