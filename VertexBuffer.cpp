#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(
    unsigned int buffer_type, std::vector<Vertex> vertices, 
    unsigned int draw_type
    ){
    glGenBuffers(1, &this->m_id);
    glBindBuffer(buffer_type, this->m_id);
    glBufferData(buffer_type, vertices.size() * sizeof(Vertex), vertices.data(), draw_type);
}

VertexBuffer::~VertexBuffer(){}

void VertexBuffer::Bind(){}

void VertexBuffer::UnBind(){}
