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
    generateIndexArrayForLines(m_indices);
}

SelectionRenderer::~SelectionRenderer() = default;

void SelectionRenderer::render(const CameraOld &camera, const std::vector<GameObject> &selectedObjects)
{
    if (m_batch == nullptr)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_LINES);
    }

    updateVertices(selectedObjects);

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);

    size_t quadCount = m_vertices.size() / 4;
    size_t indexCount = quadCount * 8;

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

        glm::vec3 bottomLeft = {centerPos.x - halfWidth, centerPos.y - halfHeight, 0.0f};
        glm::vec3 bottomRight = {centerPos.x + halfWidth, centerPos.y - halfHeight, 0.0f};
        glm::vec3 topRight = {centerPos.x + halfWidth, centerPos.y + halfHeight, 0.0f};
        glm::vec3 topLeft = {centerPos.x - halfWidth, centerPos.y + halfHeight, 0.0f};

        glm::vec4 selectionColor = glm::vec4(0.0f, 0.6f, 1.0f, 1.0f); // blue outline

        auto makeVertex = [&](const glm::vec3 &pos) -> Vertex
        {
            return Vertex{
                pos,
                selectionColor,
                glm::vec2(0.0f), // no texture
                -1.0f            // sentinel tex index
            };
        };

        m_vertices.push_back(makeVertex(topLeft));
        m_vertices.push_back(makeVertex(bottomLeft));
        m_vertices.push_back(makeVertex(bottomRight));
        m_vertices.push_back(makeVertex(topRight));
    }
}

