#include "core/components/Sprite.h"
#include "core/components/Text.h"

#include "renderer/SpriteRenderer.h"
#include "renderer/util.h"

#include "util/PathUtils.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

SpriteRenderer::SpriteRenderer()
{
    generateIndexArrayForTriangle(m_indices);
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::render(const CameraOld &camera, const std::vector<GameObject> &gameObjects)
{
    if (!m_batch)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    }

    updateVertices(gameObjects);

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

void SpriteRenderer::updateVertices(const std::vector<GameObject> &gameObjects)
{
    // clear old data
    m_vertices.clear();

    for (const GameObject &gameObject : gameObjects)
    {
        // add text vertices if object has them
        if (gameObject.hasComponent<Text>())
        {
            Text &text = gameObject.getComponent<Text>();
            text.rebuild(); // FIXME: we should only rebuild when dirty.
            std::for_each(text.vertices.begin(),
                text.vertices.end(),
                [&](Vertex &vertex)
                {
                    m_vertices.push_back(vertex);
                });
        }

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
