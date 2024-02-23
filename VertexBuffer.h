#include <vector>

#include "Draw.h"


class VertexBuffer {

private:
    unsigned int m_id;

public:
    VertexBuffer(std::vector<Vertex> vertices, unsigned int draw_type);
    ~VertexBuffer();
    void bind() const;
    void unbind() const;
};
