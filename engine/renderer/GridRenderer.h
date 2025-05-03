#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <memory>

#include "core/Camera.h"
#include "opengl/Vertex.h"
#include "opengl/Shader.h"
#include "opengl/VertexArray.h"
#include "opengl/VertexBuffer.h"

class GridRenderer
{
public:
    GridRenderer(int width, int height, int tileSize, std::shared_ptr<Camera> camera);
    ~GridRenderer();

    void render(std::shared_ptr<Camera> camera);

private:
    void generateGridLines(std::shared_ptr<Camera> camera);

    int m_width, m_height, m_tileSize;
    std::vector<Vertex> m_vertices;

    VertexArray *m_vao;
    VertexBuffer *m_vbo;
    std::shared_ptr<Shader> m_shader;
};
