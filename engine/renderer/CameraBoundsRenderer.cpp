#include "renderer/CameraBoundsRenderer.h"

#include "editor/Constants.h"
#include "renderer/util.h"

#include <glm/glm.hpp>

CameraBoundsRenderer::CameraBoundsRenderer()
{
    // Use triangle indices so we can draw thin quads instead of line primitives.
    generateIndexArrayForTriangle(m_indices);
}

CameraBoundsRenderer::~CameraBoundsRenderer() = default;

void CameraBoundsRenderer::render(const CameraOld &editorCamera, const std::vector<GameObject> & /*gameObjects*/)
{
    if (!m_batch)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    }

    updateVertices();

    if (m_vertices.empty())
    {
        return;
    }

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);

    size_t quadCount = m_vertices.size() / 4;
    size_t indexCount = quadCount * 6;

    m_batch->render(editorCamera, static_cast<int>(indexCount));
}

void CameraBoundsRenderer::updateVertices()
{
    m_vertices.clear();

    // NOTE: For now we don't look up a primary Camera component or use
    // its Transform. Instead we draw a fixed camera box from the origin
    // using the logical resolution. This approximates the gameplay view
    // without depending on GameObject placement.
    float left = 0.0f;
    float bottom = 0.0f;
    float right = LOGICAL_WIDTH;
    float top = LOGICAL_HEIGHT;

    // We draw 4 thin quads (top, bottom, left, right) instead of
    // GL_LINES so the "lines" have a controllable thickness.
    const float thickness = 2.0f; // world units

    glm::vec4 color{1.0f, 1.0f, 0.0f, 1.0f}; // yellow camera frame

    auto makeVertex = [&](const glm::vec3 &pos) -> Vertex
    {
        return Vertex{pos, color, glm::vec2(0.0f), -1};
    };

    // Top edge quad
    {
        glm::vec3 topLeft{left, top, 0.0f};
        glm::vec3 bottomLeft{left, top - thickness, 0.0f};
        glm::vec3 bottomRight{right, top - thickness, 0.0f};
        glm::vec3 topRight{right, top, 0.0f};

        m_vertices.push_back(makeVertex(topLeft));
        m_vertices.push_back(makeVertex(bottomLeft));
        m_vertices.push_back(makeVertex(bottomRight));
        m_vertices.push_back(makeVertex(topRight));
    }

    // Bottom edge quad
    {
        glm::vec3 topLeft{left, bottom + thickness, 0.0f};
        glm::vec3 bottomLeft{left, bottom, 0.0f};
        glm::vec3 bottomRight{right, bottom, 0.0f};
        glm::vec3 topRight{right, bottom + thickness, 0.0f};

        m_vertices.push_back(makeVertex(topLeft));
        m_vertices.push_back(makeVertex(bottomLeft));
        m_vertices.push_back(makeVertex(bottomRight));
        m_vertices.push_back(makeVertex(topRight));
    }

    // Left edge quad
    {
        glm::vec3 topLeft{left, top, 0.0f};
        glm::vec3 bottomLeft{left, bottom, 0.0f};
        glm::vec3 bottomRight{left + thickness, bottom, 0.0f};
        glm::vec3 topRight{left + thickness, top, 0.0f};

        m_vertices.push_back(makeVertex(topLeft));
        m_vertices.push_back(makeVertex(bottomLeft));
        m_vertices.push_back(makeVertex(bottomRight));
        m_vertices.push_back(makeVertex(topRight));
    }

    // Right edge quad
    {
        glm::vec3 topLeft{right - thickness, top, 0.0f};
        glm::vec3 bottomLeft{right - thickness, bottom, 0.0f};
        glm::vec3 bottomRight{right, bottom, 0.0f};
        glm::vec3 topRight{right, top, 0.0f};

        m_vertices.push_back(makeVertex(topLeft));
        m_vertices.push_back(makeVertex(bottomLeft));
        m_vertices.push_back(makeVertex(bottomRight));
        m_vertices.push_back(makeVertex(topRight));
    }
}
