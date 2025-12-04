#include "core/components/Sprite.h"

#include "renderer/SpriteRenderer.h"

#include <iostream>

SpriteRenderer::SpriteRenderer()
{
    generateIndexArray();
}

SpriteRenderer::~SpriteRenderer()
{
}

// TODO: should we call this renderLayer?
void SpriteRenderer::render(const CameraOld &camera, const std::vector<GameObject> &gameObjects)
{
    if (!m_batch)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    }

    updateVertices(camera, gameObjects);

    if (m_vertices.empty())
    {
        return;
    }

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);

    size_t quadCount = m_vertices.size() / 4;
    size_t indexCount = quadCount * 6;

    m_batch->render(camera, indexCount);
}

void SpriteRenderer::updateVertices(const CameraOld &camera, const std::vector<GameObject> &gameObjects)
{
    // clear old data
    m_vertices.clear();

    for (const GameObject &gameObject : gameObjects)
    {
        if (!gameObject.hasComponent<Sprite>())
        {
            continue;
        }

        Sprite &sprite = gameObject.getComponent<Sprite>();

        std::array<glm::vec3, 4> quad = gameObject.getWorldCoordinateQuad();
        auto texCoords = sprite.getNormalizedTextureCoordinates();
        float texUnit = (float)sprite.texture->getTextureUnit();

        // TODO: this should be moved to the base renderer class or should be a common utility
        auto makeVertex = [&](const glm::vec3 &pos, const glm::vec4 &color, const glm::vec2 &uv, float t)
        {
            return Vertex{pos, color, uv, t};
        };

        m_vertices.push_back(makeVertex(quad[0], sprite.color, texCoords[0], texUnit));
        m_vertices.push_back(makeVertex(quad[1], sprite.color, texCoords[1], texUnit));
        m_vertices.push_back(makeVertex(quad[2], sprite.color, texCoords[2], texUnit));
        m_vertices.push_back(makeVertex(quad[3], sprite.color, texCoords[3], texUnit));
    }
}

// TODO: can we make this a utility of or part of the base class?
// The pattern of the indices is the same for each quad
// 0, 1, 2, 0, 3, 2     // then by adding 4 to each index
// +4,+4,+4,+4,+4,+4
// 4, 5, 6, 4, 7, 6     // we get this one and so on.
// The for-loop below generates the indices for each quad
void SpriteRenderer::generateIndexArray()
{
    // m_indices.reserve(m_batch->BATCH_SIZE * m_batch->INDICES_PER_QUAD);
    // for (int i = 0; i < m_batch->BATCH_SIZE; i++)
    // {
    m_indices.resize(1000 * 6);
    for (int i = 0; i < 1000; i++)
    {
        m_indices[i * 6] = i * 4;
        m_indices[i * 6 + 1] = i * 4 + 1;
        m_indices[i * 6 + 2] = i * 4 + 2;
        m_indices[i * 6 + 3] = i * 4;
        m_indices[i * 6 + 4] = i * 4 + 3;
        m_indices[i * 6 + 5] = i * 4 + 2;
    }
    std::cout << "index size: " << m_indices.size() << std::endl;
}
