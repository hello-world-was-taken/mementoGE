#include <glm/glm.hpp>

#include "renderer/GridRenderer.h"
#include "core/Resource.h"
#include "opengl/Vertex.h"
#include "util/log_error.h"

GridRenderer::GridRenderer(int width, int height, int tileSize, std::shared_ptr<Camera> camera)
    : m_width(width), m_height(height), m_tileSize(tileSize)
{
    generateGridLines(camera);

    m_vao = new VertexArray();
    m_vao->bind();

    m_vbo = new VertexBuffer(m_vertices.size() * sizeof(Vertex), GL_DYNAMIC_DRAW);
    m_vbo->bind();

    m_vao->attachVertexAttribute(VertexAttribute{3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0}); // position

    m_vbo->updateBufferData(m_vertices);

    m_shader = Resource::getShaderProgram("../assets/shader/grid_vertex.shader", "../assets/shader/grid_fragment.shader");
}

GridRenderer::~GridRenderer()
{
    delete m_vbo;
    delete m_vao;
}

void GridRenderer::generateGridLines(std::shared_ptr<Camera> camera)
{
    m_vertices.clear();

    // Default color or texcoord, unused in this case
    glm::vec4 color = glm::vec4(1.0f); // optional
    glm::vec2 texCoord = glm::vec2(0.0f);
    float texIndex = 0.0f;

    // to support grid rendering while zooming and camera move
    glm::mat4 invViewProj = glm::inverse(camera->getProjectionMatrix() * camera->getViewMatrix());

    // TODO: review this more!
    glm::vec4 corners[2] = {
        invViewProj * glm::vec4(-1, -1, 0, 1), // bottom-left NDC
        invViewProj * glm::vec4(1, 1, 0, 1),   // top-right NDC
    };

    glm::vec2 bottomLeft = glm::vec2(corners[0]) / corners[0].w;
    glm::vec2 topRight = glm::vec2(corners[1]) / corners[1].w;

    // The same snapping logic we used in MouseActionController.cpp
    // They should be kept in sync
    int startX = static_cast<int>(std::floor(bottomLeft.x / m_tileSize)) * m_tileSize;
    int endX = static_cast<int>(std::ceil(topRight.x / m_tileSize)) * m_tileSize;

    int startY = static_cast<int>(std::floor(bottomLeft.y / m_tileSize)) * m_tileSize;
    int endY = static_cast<int>(std::ceil(topRight.y / m_tileSize)) * m_tileSize;

    m_vertices.clear();
    for (int x = startX; x <= endX; x += m_tileSize)
    {
        m_vertices.emplace_back(Vertex{glm::vec3(x, startY, 0), color, texCoord, texIndex});
        m_vertices.emplace_back(Vertex{glm::vec3(x, endY, 0), color, texCoord, texIndex});
    }
    for (int y = startY; y <= endY; y += m_tileSize)
    {
        m_vertices.emplace_back(Vertex{glm::vec3(startX, y, 0), color, texCoord, texIndex});
        m_vertices.emplace_back(Vertex{glm::vec3(endX, y, 0), color, texCoord, texIndex});
    }
}

void GridRenderer::render(std::shared_ptr<Camera> camera)
{
    m_shader->use();
    m_shader->setUniform4fv("u_view_matrix", camera->getViewMatrix());
    m_shader->setUniform4fv("u_projection_matrix", camera->getProjectionMatrix());

    // TODO: we can avoid generating grids by caching previous camera position
    generateGridLines(camera);

    m_vao->bind();
    // TODO: why was binding vao not enough here?
    m_vbo->bind();
    m_vbo->updateBufferData(m_vertices);
    m_vbo->unbind();

    glClearError();
    glDrawArrays(GL_LINES, 0, m_vertices.size());
    glCheckError("glDrawLines", __FILE__, __LINE__);

    m_vao->unbind();
}
