#include "core/components/Transform.h"
#include "core/components/EntityInfo.h"

#include "renderer/SelectionRenderer.h"

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

SelectionRenderer::SelectionRenderer()
{
    generateIndexArray();
}

SelectionRenderer::~SelectionRenderer() = default;

void SelectionRenderer::render(
    const CameraOld &camera, const std::vector<std::reference_wrapper<GameObject>> &selectedObjects)
{
    if (m_batch == nullptr)
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_LINES);

    updateVertices(selectedObjects);

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);
    m_batch->render(camera, m_vertices.size() / 4 * 8); // TODO: remove magic numbers
}

void SelectionRenderer::updateVertices(const std::vector<std::reference_wrapper<GameObject>> &selectedObjects)
{
    m_vertices.clear();

    for (const GameObject &obj : selectedObjects)
    {
        if (!obj.hasComponent<Transform>())
            continue;

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
                pos, selectionColor,
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

void SelectionRenderer::generateIndexArray()
{
    const int maxBoxes = 1000;
    m_indices.resize(maxBoxes * 8);

    for (int i = 0; i < maxBoxes; ++i)
    {
        // topLeft → bottomLeft
        m_indices[i * 8 + 0] = i * 4 + 0;
        m_indices[i * 8 + 1] = i * 4 + 1;

        // bottomLeft → bottomRight
        m_indices[i * 8 + 2] = i * 4 + 1;
        m_indices[i * 8 + 3] = i * 4 + 2;

        // bottomRight → topRight
        m_indices[i * 8 + 4] = i * 4 + 2;
        m_indices[i * 8 + 5] = i * 4 + 3;

        // topRight → topLeft
        m_indices[i * 8 + 6] = i * 4 + 3;
        m_indices[i * 8 + 7] = i * 4 + 0;
    }
}
