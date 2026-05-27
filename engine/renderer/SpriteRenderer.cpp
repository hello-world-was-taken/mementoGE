#include "core/components/ParticleEmitter.h"
#include "core/components/ShaderEffectRequest.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"

#include "core/GlResourceManager.h"

#include "renderer/SpriteRenderer.h"
#include "renderer/util.h"

#include "util/PathUtils.h"
#include "util/LogChannels.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <sawyer/Logger.h>

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

    m_batch->setIndexData(m_indices);

    // Preserve the existing behavior where the last requested shader is used
    // for the complete sprite pass, including passes split by texture capacity.
    m_currentShader = nullptr;
    for (const GameObject &gameObject : gameObjects)
    {
        if (!gameObject.hasComponent<ShaderEffectRequest>())
        {
            continue;
        }

        const ShaderEffectRequest &effectRequest = gameObject.getComponent<ShaderEffectRequest>();
        m_currentShader = GlResourceManager::instance().getShaderProgram(
            effectRequest.vertexShaderPath, effectRequest.fragmentShaderPath);

        LOG_DEBUG(LogChannel::Renderer,
            "ShaderEffectRequest found with vertex shader '{}' and fragment shader '{}'",
            effectRequest.vertexShaderPath,
            effectRequest.fragmentShaderPath);
    }

    m_vertices.clear();
    std::vector<std::shared_ptr<Texture>> batchTextures;

    auto flush = [&]()
    {
        if (m_vertices.empty())
        {
            return;
        }

        m_batch->setVertexData(m_vertices);
        m_batch->render(camera,
            static_cast<int>((m_vertices.size() / VERTEX_PER_QUAD) * INDICES_PER_QUAD),
            m_currentShader,
            batchTextures);
        m_vertices.clear();
        batchTextures.clear();
    };

    auto appendQuad = [&](const Vertex *vertices, const std::shared_ptr<Texture> &texture)
    {
        auto textureIt = texture
            ? std::find(batchTextures.begin(), batchTextures.end(), texture)
            : batchTextures.end();
        const bool needsTextureSlot = texture && textureIt == batchTextures.end();
        if (m_vertices.size() / VERTEX_PER_QUAD >= BATCH_SIZE ||
            (needsTextureSlot && batchTextures.size() >= MAX_TEXTURE_SLOTS_PER_BATCH))
        {
            flush();
            textureIt = batchTextures.end();
        }

        int textureSlot = -1;
        if (texture)
        {
            if (textureIt == batchTextures.end())
            {
                batchTextures.push_back(texture);
                textureSlot = static_cast<int>(batchTextures.size() - 1);
            }
            else
            {
                textureSlot = static_cast<int>(std::distance(batchTextures.begin(), textureIt));
            }
        }

        for (int i = 0; i < VERTEX_PER_QUAD; ++i)
        {
            Vertex vertex = vertices[i];
            vertex.texSlot = textureSlot;
            m_vertices.push_back(vertex);
        }
    };

    for (const GameObject &gameObject : gameObjects)
    {
        // text
        if (gameObject.hasComponent<Text>())
        {
            Text &text = gameObject.getComponent<Text>();
            Transform &transform = gameObject.getComponent<Transform>();
            text.rebuild(transform); // FIXME: we should only rebuild when dirty.
            for (size_t i = 0; i + VERTEX_PER_QUAD <= text.vertices.size(); i += VERTEX_PER_QUAD)
            {
                appendQuad(text.vertices.data() + i, text.font->atlasTexture);
            }
        }

        // sprite
        if (gameObject.hasComponent<Sprite>())
        {
            Sprite &sprite = gameObject.getComponent<Sprite>();

            std::array<glm::vec3, 4> quad = gameObject.getWorldCoordinateQuad();
            auto texCoords = sprite.getNormalizedTextureCoordinates();
            // TODO: this should be moved to the base renderer class or should be a common utility
            auto makeVertex = [&](const glm::vec3 &pos, const glm::vec4 &color, const glm::vec2 &uv)
            {
                return Vertex{pos, color, uv, -1};
            };

            std::array<Vertex, VERTEX_PER_QUAD> vertices = {
                makeVertex(quad[0], sprite.color, texCoords[0]),
                makeVertex(quad[1], sprite.color, texCoords[1]),
                makeVertex(quad[2], sprite.color, texCoords[2]),
                makeVertex(quad[3], sprite.color, texCoords[3]),
            };
            appendQuad(vertices.data(), sprite.texture);
        }

        // particle
        if (gameObject.hasComponent<ParticleEmitter>())
        {
            const auto &emitter = gameObject.getComponent<ParticleEmitter>();
            // TODO: for now, lets just use color. -1 is treated as a special case in the fragment shader
            auto makeVertex = [&](const glm::vec3 &pos, const glm::vec4 &color, const glm::vec2 &uv, int t)
            {
                return Vertex{pos, color, uv, t};
            };

            for (const Particle &p : emitter.particles)
            {
                auto quad = makeQuadFromCenter(p.pos, p.size);

                std::array<Vertex, VERTEX_PER_QUAD> vertices = {
                    makeVertex(quad[0], p.color, {0.0f, 0.0f}, -1),
                    makeVertex(quad[1], p.color, {0.0f, 0.0f}, -1),
                    makeVertex(quad[2], p.color, {0.0f, 0.0f}, -1),
                    makeVertex(quad[3], p.color, {0.0f, 0.0f}, -1),
                };
                appendQuad(vertices.data(), nullptr);
            }
        }

    }

    flush();
    m_currentShader = nullptr; // reset to default shader after rendering
}
