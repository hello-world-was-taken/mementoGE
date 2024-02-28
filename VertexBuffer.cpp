#include "VertexBuffer.h"


VertexBuffer::VertexBuffer(
    std::vector<Vertex> vertices, 
    unsigned int draw_type
    ){
    glGenBuffers(1, &this->m_id);
    glBindBuffer(GL_ARRAY_BUFFER, this->m_id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), draw_type);
}


VertexBuffer::~VertexBuffer(){
    // std::cout << "calling VertexBuffer destructor" << std::endl;
    // glDeleteBuffers(1, &this->m_id);
}


void VertexBuffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, this->m_id);
}


void VertexBuffer::unbind() const{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void VertexBuffer::addAttribute(VertexAttribute vertex_attribute){
    this->m_vertex_attributes.push_back(vertex_attribute);
}


void VertexBuffer::setAttributePointers() const{
    for (unsigned int i = 0; i < this->m_vertex_attributes.size(); i++){
        VertexAttribute vertex_attribute = this->m_vertex_attributes[i];
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(
            i, 
            vertex_attribute.m_vector_length, 
            vertex_attribute.m_type, 
            vertex_attribute.m_normalized, 
            vertex_attribute.m_stride,
            (void*)vertex_attribute.m_offset
        );
    }
}
