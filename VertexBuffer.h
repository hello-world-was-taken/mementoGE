#include <vector>

#include "./opengl/draw.h"


class VertexBuffer {

private:
    unsigned int m_id;

public:
    VertexBuffer(unsigned int type, std::vector<Vertex> vertices, unsigned int draw_type);
    ~VertexBuffer();
    void Bind();
    void UnBind();
};
