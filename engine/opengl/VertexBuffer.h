#pragma once

#include <vector>

#include "Vertex.h"
#include "VertexAttribute.h"

/*
* VBOs handles the creation of memory on the GPU. This abstraction will create
* a memory of certain size as well us the draw type - triangle, line and so on.
* OpenGL is a state machine. So it operates on the currently bound VBO. We can
* bind multiple VBOs at the same time as long as they are different buffer types.
*/
class VertexBuffer
{
public:
    VertexBuffer() = default;
    VertexBuffer(unsigned int buffer_size, unsigned int draw_type);
    ~VertexBuffer();

    void bind() const;
    void unbind() const;

    void updateBufferData(std::vector<Vertex> vertices);

private:
    unsigned int m_id;
    std::vector<VertexAttribute> m_vertex_attributes;
};
