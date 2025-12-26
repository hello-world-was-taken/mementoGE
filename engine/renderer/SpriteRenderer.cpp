#include "core/components/ParticleEmitter.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"

#include "renderer/SpriteRenderer.h"
#include "renderer/util.h"

#include "util/PathUtils.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

static std::array<glm::vec3, 4> makeQuadFromCenter(const glm::vec3 &center, float size)
{
    float h = size * 0.5f;
    return {
        glm::vec3(center.x - h, center.y - h, center.z),
        glm::vec3(center.x + h, center.y - h, center.z),
        glm::vec3(center.x + h, center.y + h, center.z),
        glm::vec3(center.x - h, center.y + h, center.z),
    };
}

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
        // text
        if (gameObject.hasComponent<Text>())
        {
            Text &text = gameObject.getComponent<Text>();
            Transform &transform = gameObject.getComponent<Transform>();
            text.rebuild(transform); // FIXME: we should only rebuild when dirty.
            std::for_each(text.vertices.begin(),
                text.vertices.end(),
                [&](Vertex &vertex)
                {
                    m_vertices.push_back(vertex);
                });
        }

        // sprite
        if (gameObject.hasComponent<Sprite>())
        {
            Sprite &sprite = gameObject.getComponent<Sprite>();

            std::array<glm::vec3, 4> quad = gameObject.getWorldCoordinateQuad();
            auto texCoords = sprite.getNormalizedTextureCoordinates();
            int texSlot = sprite.texture->getTextureSlot();
            sprite.texture->bind();

            // TODO: this should be moved to the base renderer class or should be a common utility
            auto makeVertex = [&](const glm::vec3 &pos, const glm::vec4 &color, const glm::vec2 &uv, int t)
            {
                return Vertex{pos, color, uv, t};
            };

            m_vertices.push_back(makeVertex(quad[0], sprite.color, texCoords[0], texSlot));
            m_vertices.push_back(makeVertex(quad[1], sprite.color, texCoords[1], texSlot));
            m_vertices.push_back(makeVertex(quad[2], sprite.color, texCoords[2], texSlot));
            m_vertices.push_back(makeVertex(quad[3], sprite.color, texCoords[3], texSlot));
        }

        // particle
        if (gameObject.hasComponent<ParticleEmitter>())
        {
            const auto &emitter = gameObject.getComponent<ParticleEmitter>();
            // TODO: for now, lets just use color. -1 is treated as a special case in the fragment shader
            int texSlot = -1;

            auto makeVertex = [&](const glm::vec3 &pos, const glm::vec4 &color, const glm::vec2 &uv, int t)
            {
                return Vertex{pos, color, uv, t};
            };

            for (const Particle &p : emitter.particles)
            {
                auto quad = makeQuadFromCenter(p.pos, p.size);

                m_vertices.push_back(makeVertex(quad[0], p.color, {0.0f, 0.0f}, texSlot));
                m_vertices.push_back(makeVertex(quad[1], p.color, {0.0f, 0.0f}, texSlot));
                m_vertices.push_back(makeVertex(quad[2], p.color, {0.0f, 0.0f}, texSlot));
                m_vertices.push_back(makeVertex(quad[3], p.color, {0.0f, 0.0f}, texSlot));
            }
        }
    }
}
