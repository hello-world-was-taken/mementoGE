#include "Indexbuffer.h"
#include <GL/glew.h>


IndexBuffer::IndexBuffer(
    const unsigned int *indices,
    const int length,
    unsigned int draw_type
    ) {
    glGenBuffers(1, &this->m_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, length * sizeof(indices), indices, draw_type);
}

IndexBuffer::~IndexBuffer() {}

void IndexBuffer::bind() const {}

void IndexBuffer::unbind() const{}