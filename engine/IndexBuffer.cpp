#include "Indexbuffer.h"
#include <GL/glew.h>
#include <iostream>


IndexBuffer::IndexBuffer(
    const unsigned int *indices,
    const int length,
    unsigned int draw_type
    ) {
    glGenBuffers(1, &this->m_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, length * sizeof(indices), indices, draw_type);
}


// TODO: The destructor should be the VAO destructor is called not when the object is destroyed
IndexBuffer::~IndexBuffer() {
    // std::cout << "calling IndexBuffer destructor" << std::endl;
    // glDeleteBuffers(1, &this->m_id);
}

void IndexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_id);
}

void IndexBuffer::unbind() const{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}