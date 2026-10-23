#include "renderer/GizmoRenderer.h"

#include "core/components/Transform.h"
#include "editor/Constants.h"
#include "renderer/util.h"

namespace
{
constexpr float AXIS_LENGTH_PIXELS = 50.0f;
constexpr float SHAFT_THICKNESS_PIXELS = 3.0f;
constexpr float ARROW_LENGTH_PIXELS = 10.0f;
constexpr float ARROW_HALF_WIDTH_PIXELS = 6.0f;
constexpr float CENTER_SIZE_PIXELS = 7.0f;

const glm::vec4 X_AXIS_COLOR{0.902f, 0.286f, 0.286f, 1.0f};
const glm::vec4 Y_AXIS_COLOR{0.302f, 0.780f, 0.412f, 1.0f};
const glm::vec4 CENTER_COLOR{0.920f, 0.920f, 0.920f, 1.0f};
} // namespace

GizmoRenderer::GizmoRenderer()
{
    generateIndexArrayForTriangle(m_indices);
}

GizmoRenderer::~GizmoRenderer() = default;

void GizmoRenderer::render(const Camera &camera, double elapsedTime, const std::vector<GameObject> &selectedObjects)
{
    updateVertices(camera, selectedObjects);
    if (m_vertices.empty())
    {
        return;
    }

    if (!m_batch)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    }

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);
    m_batch->render(camera, elapsedTime, static_cast<int>((m_vertices.size() / 4) * INDICES_PER_QUAD));
}

void GizmoRenderer::updateVertices(const Camera &camera, const std::vector<GameObject> &selectedObjects)
{
    m_vertices.clear();
    if (selectedObjects.size() != 1 || !selectedObjects.front().hasComponent<Transform>())
    {
        return;
    }

    const Transform &transform = selectedObjects.front().getComponent<Transform>();
    const glm::vec2 center{transform.position.x, transform.position.y};

    // EditorCamera maps one logical screen pixel to this many world units.
    // Scaling by zoom keeps the gizmo visually constant while zooming.
    const float worldUnitsPerPixel = (LOGICAL_WIDTH / WINDOW_WIDTH) * camera.zoom;
    const float axisLength = AXIS_LENGTH_PIXELS * worldUnitsPerPixel;
    const float halfThickness = SHAFT_THICKNESS_PIXELS * worldUnitsPerPixel * 0.5f;
    const float arrowLength = ARROW_LENGTH_PIXELS * worldUnitsPerPixel;
    const float arrowHalfWidth = ARROW_HALF_WIDTH_PIXELS * worldUnitsPerPixel;
    const float centerHalfSize = CENTER_SIZE_PIXELS * worldUnitsPerPixel * 0.5f;

    appendQuad({center.x, center.y - halfThickness},
        {center.x + axisLength - arrowLength, center.y + halfThickness},
        X_AXIS_COLOR);
    appendTriangle({center.x + axisLength, center.y},
        {center.x + axisLength - arrowLength, center.y - arrowHalfWidth},
        {center.x + axisLength - arrowLength, center.y + arrowHalfWidth},
        X_AXIS_COLOR);

    appendQuad({center.x - halfThickness, center.y},
        {center.x + halfThickness, center.y + axisLength - arrowLength},
        Y_AXIS_COLOR);
    appendTriangle({center.x, center.y + axisLength},
        {center.x - arrowHalfWidth, center.y + axisLength - arrowLength},
        {center.x + arrowHalfWidth, center.y + axisLength - arrowLength},
        Y_AXIS_COLOR);

    appendQuad(center - glm::vec2{centerHalfSize}, center + glm::vec2{centerHalfSize}, CENTER_COLOR);
}

void GizmoRenderer::appendQuad(glm::vec2 min, glm::vec2 max, const glm::vec4 &color)
{
    m_vertices.emplace_back(Vertex{{min.x, max.y, 0.0f}, color, {0.0f, 0.0f}, -1});
    m_vertices.emplace_back(Vertex{{min.x, min.y, 0.0f}, color, {0.0f, 0.0f}, -1});
    m_vertices.emplace_back(Vertex{{max.x, min.y, 0.0f}, color, {0.0f, 0.0f}, -1});
    m_vertices.emplace_back(Vertex{{max.x, max.y, 0.0f}, color, {0.0f, 0.0f}, -1});
}

void GizmoRenderer::appendTriangle(
    glm::vec2 first, glm::vec2 second, glm::vec2 third, const glm::vec4 &color)
{
    m_vertices.emplace_back(Vertex{{first.x, first.y, 0.0f}, color, {0.0f, 0.0f}, -1});
    m_vertices.emplace_back(Vertex{{second.x, second.y, 0.0f}, color, {0.0f, 0.0f}, -1});
    m_vertices.emplace_back(Vertex{{third.x, third.y, 0.0f}, color, {0.0f, 0.0f}, -1});
    // RenderBatch is quad-indexed. Repeating the final point makes the second
    // generated triangle degenerate while retaining the desired first triangle.
    m_vertices.emplace_back(Vertex{{third.x, third.y, 0.0f}, color, {0.0f, 0.0f}, -1});
}
