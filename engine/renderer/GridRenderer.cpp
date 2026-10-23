#include "renderer/GridRenderer.h"
#include "core/GlResourceManager.h"
#include "opengl/Vertex.h"
#include "renderer/util.h"

#include "editor/Constants.h"
#include "util/LogChannels.h"
#include "util/PathUtils.h"

#include <filesystem>
#include <glm/glm.hpp>
#include <sawyer/Logger.h>

namespace
{
void appendLineQuad(std::vector<Vertex> &vertices, const glm::vec2 &start, const glm::vec2 &end, float thickness)
{
    const glm::vec2 direction = end - start;
    const glm::vec2 normal = glm::normalize(glm::vec2{-direction.y, direction.x}) * (thickness * 0.5f);
    const glm::vec4 color{1.0f};
    const glm::vec2 texCoord{0.0f};
    constexpr int texSlot = -1;

    vertices.emplace_back(Vertex{glm::vec3(start + normal, 0.0f), color, texCoord, texSlot});
    vertices.emplace_back(Vertex{glm::vec3(start - normal, 0.0f), color, texCoord, texSlot});
    vertices.emplace_back(Vertex{glm::vec3(end - normal, 0.0f), color, texCoord, texSlot});
    vertices.emplace_back(Vertex{glm::vec3(end + normal, 0.0f), color, texCoord, texSlot});
}
} // namespace

GridRenderer::GridRenderer()
{
    LOG_DEBUG(LogChannel::Renderer, "Current directory: {}", std::filesystem::current_path().string());
    m_shader = GlResourceManager::instance().getShaderProgram(
        getEngineAssetPath("shader/grid_vertex.shader"), getEngineAssetPath("shader/grid_fragment.shader"));

    generateIndexArrayForTriangle(m_indices);
    m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    m_batch->setIndexData(m_indices);
}

GridRenderer::~GridRenderer()
{
}

void GridRenderer::generateGridLines(const Camera &camera, int tileSize)
{
    m_vertices.clear();

    // Compute world-space corners by unprojecting the NDC corners
    // [-1, -1] and [1, 1]. This lets us build a grid that always
    // covers the visible region, so it appears to extend infinitely
    // as the camera pans and zooms.
    glm::mat4 invViewProj = glm::inverse(camera.projection * camera.view);
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

    // Match the old one-pixel GL_LINES appearance across editor zoom levels.
    const float lineThickness = (LOGICAL_WIDTH / WINDOW_WIDTH) * camera.zoom;

    for (int x = startX; x <= endX; x += tileSize)
    {
        appendLineQuad(m_vertices, {x, startY}, {x, endY}, lineThickness);
    }
    for (int y = startY; y <= endY; y += tileSize)
    {
        appendLineQuad(m_vertices, {startX, y}, {endX, y}, lineThickness);
    }
}

void GridRenderer::render(const Camera &camera, double elapsedTime, int tileSize)
{
    m_shader->use();
    m_shader->setUniform4fv("u_view_matrix", camera.view);
    m_shader->setUniform4fv("u_projection_matrix", camera.projection);

    // TODO: we can avoid generating grids by caching previous camera position
    generateGridLines(camera, tileSize);

    m_batch->setVertexData(m_vertices);

    const size_t quadCount = m_vertices.size() / 4;
    m_batch->render(camera, elapsedTime, static_cast<int>(quadCount * INDICES_PER_QUAD), m_shader);
}
