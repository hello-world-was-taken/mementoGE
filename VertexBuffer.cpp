#include <GL/glew.h>
#include "VertexBuffer.h"


VertexBuffer::VertexBuffer(
    std::vector<Vertex> vertices, 
    unsigned int draw_type
    ){
    glGenBuffers(1, &this->m_id);
    glBindBuffer(GL_ARRAY_BUFFER, this->m_id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), draw_type);
}

VertexBuffer::~VertexBuffer(){}

void VertexBuffer::bind() const {}

void VertexBuffer::unbind() const{}
