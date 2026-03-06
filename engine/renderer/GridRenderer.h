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
    GridRenderer(int width, int height, int tileSize, const CameraOld &camera);
    ~GridRenderer();

    void render(const CameraOld &camera);

private:
    void generateGridLines(const CameraOld &camera);

    int m_width, m_height, m_tileSize;
    std::vector<Vertex> m_vertices;

    std::unique_ptr<VertexArray> m_vao;
    std::unique_ptr<VertexBuffer> m_vbo;
    std::shared_ptr<Shader> m_shader;
};
