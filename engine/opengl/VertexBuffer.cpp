#include "VertexBuffer.h"
#include "Vertex.h"
#include "VertexAttribute.h"
#include "util/LogChannels.h"

#include <sawyer/Logger.h>

VertexBuffer::VertexBuffer(
    unsigned int buffer_size,
    unsigned int draw_type)
{
    glGenBuffers(1, &this->m_id);
    glBindBuffer(GL_ARRAY_BUFFER, this->m_id);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr, draw_type);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexBuffer::~VertexBuffer()
{
    LOG_DEBUG(LogChannel::OpenGL, "Calling VertexBuffer destructor");
    glDeleteBuffers(1, &this->m_id);
}

void VertexBuffer::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, this->m_id);
}

void VertexBuffer::unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::updateBufferData(std::vector<Vertex> vertices)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
