#include <iostream>
#include <GL/glew.h>

#include "Indexbuffer.h"

IndexBuffer::IndexBuffer(
    const int length,
    unsigned int draw_type)
{
    glGenBuffers(1, &this->m_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, length * sizeof(unsigned int), nullptr, draw_type);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// TODO: The destructor should be the VAO destructor is called not when the object is destroyed
IndexBuffer::~IndexBuffer()
{
    std::cout << "calling IndexBuffer destructor" << std::endl;
    glDeleteBuffers(1, &this->m_id);
}

void IndexBuffer::bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_id);
}

void IndexBuffer::unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::updateIndicesData(const unsigned int *data, int count) const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, count * sizeof(unsigned int), data);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}