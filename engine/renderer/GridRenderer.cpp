#include "renderer/GridRenderer.h"
#include "core/GlResourceManager.h"
#include "opengl/Vertex.h"

#include "util/PathUtils.h"
#include "util/log_error.h"

#include <filesystem>
#include <glm/glm.hpp>
#include <stdexcept>

GridRenderer::GridRenderer()
{
    m_vao = std::make_unique<VertexArray>();
    m_vao->bind();

    // Allocate a reasonable default buffer size; it will be updated each frame.
    // This avoids tying capacity to a specific camera/config at construction.
    constexpr size_t DEFAULT_MAX_VERTICES = 4096;
    m_vbo = std::make_unique<VertexBuffer>(DEFAULT_MAX_VERTICES * sizeof(Vertex), GL_DYNAMIC_DRAW);
    m_vbo->bind();

    m_vao->attachVertexAttribute(VertexAttribute{3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0}); // position

    std::cout << "Curr directory: " << std::filesystem::current_path() << std::endl;
    m_shader = GlResourceManager::instance().getShaderProgram(
        getFilePath("shader/grid_vertex.shader"), getFilePath("shader/grid_fragment.shader"));
}

GridRenderer::~GridRenderer()
{
}

void GridRenderer::generateGridLines(const CameraOld &camera, int tileSize)
{
    m_vertices.clear();

    // Default color or texcoord, unused in this case
    glm::vec4 color = glm::vec4(1.0f); // optional
    glm::vec2 texCoord = glm::vec2(0.0f);
    int texSlot = 0;

    // Compute world-space corners by unprojecting the NDC corners
    // [-1, -1] and [1, 1]. This lets us build a grid that always
    // covers the visible region, so it appears to extend infinitely
    // as the camera pans and zooms.
    glm::mat4 invViewProj = glm::inverse(camera.getProjectionMatrix() * camera.getViewMatrix());
    glm::vec4 corners[2] = {
        invViewProj * glm::vec4(-1, -1, 0, 1), // bottom-left NDC
        invViewProj * glm::vec4(1, 1, 0, 1),   // top-right NDC
    };

    glm::vec2 bottomLeft = glm::vec2(corners[0]) / corners[0].w;
    glm::vec2 topRight = glm::vec2(corners[1]) / corners[1].w;

    // The same snapping logic we used in EditorMouseController.cpp
    // They should be kept in sync
    int startX = static_cast<int>(std::floor(bottomLeft.x / tileSize)) * tileSize;
    int endX = static_cast<int>(std::ceil(topRight.x / tileSize)) * tileSize;

    int startY = static_cast<int>(std::floor(bottomLeft.y / tileSize)) * tileSize;
    int endY = static_cast<int>(std::ceil(topRight.y / tileSize)) * tileSize;

    m_vertices.clear();
    for (int x = startX; x <= endX; x += tileSize)
    {
        m_vertices.emplace_back(Vertex{glm::vec3(x, startY, 0), color, texCoord, texSlot});
        m_vertices.emplace_back(Vertex{glm::vec3(x, endY, 0), color, texCoord, texSlot});
    }
    for (int y = startY; y <= endY; y += tileSize)
    {
        m_vertices.emplace_back(Vertex{glm::vec3(startX, y, 0), color, texCoord, texSlot});
        m_vertices.emplace_back(Vertex{glm::vec3(endX, y, 0), color, texCoord, texSlot});
    }
}

void GridRenderer::render(const CameraOld &camera, int tileSize)
{
    m_shader->use();
    m_shader->setUniform4fv("u_view_matrix", camera.getViewMatrix());
    m_shader->setUniform4fv("u_projection_matrix", camera.getProjectionMatrix());

    // TODO: we can avoid generating grids by caching previous camera position
    generateGridLines(camera, tileSize);

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
