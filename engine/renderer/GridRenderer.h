#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "core/Camera.h"
#include "opengl/Shader.h"
#include "opengl/Vertex.h"
#include "opengl/VertexArray.h"
#include "opengl/VertexBuffer.h"

class GridRenderer
{
public:
    GridRenderer();
    ~GridRenderer();

    // Render grid lines using the given camera and tile size.
    // Tile size should be kept in sync with editor snap settings.
    void render(const CameraOld &camera, int tileSize);

private:
    void generateGridLines(const CameraOld &camera, int tileSize);

    std::vector<Vertex> m_vertices;

    std::unique_ptr<VertexArray> m_vao;
    std::unique_ptr<VertexBuffer> m_vbo;
    std::shared_ptr<Shader> m_shader;
};
