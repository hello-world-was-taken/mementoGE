#include "core/components/EntityInfo.h"
#include "core/components/Transform.h"

#include "renderer/SelectionRenderer.h"
#include "renderer/util.h"

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

SelectionRenderer::SelectionRenderer()
{
    generateIndexArrayForTriangle(m_indices);
}

SelectionRenderer::~SelectionRenderer() = default;

void SelectionRenderer::render(const CameraOld &camera, const std::vector<GameObject> &selectedObjects)
{
    if (m_batch == nullptr)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    }

    updateVertices(selectedObjects);

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);

    size_t quadCount = m_vertices.size() / 4;
    size_t indexCount = quadCount * INDICES_PER_QUAD;

    m_batch->render(camera, indexCount);
}

void SelectionRenderer::updateVertices(const std::vector<GameObject> &selectedObjects)
{
    m_vertices.clear();

    for (const GameObject &obj : selectedObjects)
    {
        if (!obj.hasComponent<Transform>())
        {
            continue;
        }

        EntityInfo &entityInfo = obj.getComponent<EntityInfo>();
        const Transform &transform = obj.getComponent<Transform>();
        glm::vec2 size = {entityInfo.width, entityInfo.height};

        float halfWidth = size.x * 0.5f;
        float halfHeight = size.y * 0.5f;

        glm::vec2 centerPos = glm::vec2{transform.position.x, transform.position.y};

        const glm::vec4 selectionColor{0.302f, 0.522f, 0.780f, 1.0f}; // softened editor blue
        constexpr float thickness = 1.0f;
        const float left = centerPos.x - halfWidth;
        const float right = centerPos.x + halfWidth;
        const float bottom = centerPos.y - halfHeight;
        const float top = centerPos.y + halfHeight;

        auto appendQuad = [&](float quadLeft, float quadBottom, float quadRight, float quadTop)
        {
            m_vertices.emplace_back(Vertex{{quadLeft, quadTop, 0.0f}, selectionColor, {0.0f, 0.0f}, -1});
            m_vertices.emplace_back(Vertex{{quadLeft, quadBottom, 0.0f}, selectionColor, {0.0f, 0.0f}, -1});
            m_vertices.emplace_back(Vertex{{quadRight, quadBottom, 0.0f}, selectionColor, {0.0f, 0.0f}, -1});
            m_vertices.emplace_back(Vertex{{quadRight, quadTop, 0.0f}, selectionColor, {0.0f, 0.0f}, -1});
        };

        appendQuad(left, top - thickness, right, top);
        appendQuad(left, bottom, right, bottom + thickness);
        appendQuad(left, bottom, left + thickness, top);
        appendQuad(right - thickness, bottom, right, top);
    }
}
