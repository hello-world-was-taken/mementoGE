#include "renderer/Renderer2D.h"
#include "renderer/util.h"

#include "core/components/Camera.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/RenderLayer.h"
#include "core/components/ShaderEffectRequest.h"
#include "core/components/Sprite.h"
#include "core/components/Text.h"

#include "core/GameObject.h"
#include "core/GlResourceManager.h"

#include <vector>

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

Renderer2D::Renderer2D()
{
}

void Renderer2D::queueFrameBufferResize(int width, int height)
{
    if (width == m_width && height == m_height)
    {
        return;
    }

    m_width = width;
    m_height = height;

    m_finalFBO.queueSizeUpdate(width, height);
}

void Renderer2D::setRendererEnablement(RendererType enablements)
{
    m_enabledRenderers = enablements;
}

unsigned int Renderer2D::getColorTexture()
{
    return m_finalFBO.getColorTexture();
}

void Renderer2D::renderScene(const CameraOld &camera, const std::vector<GameObject> &objects)
{
    m_finalFBO.resize();

    renderLayers(camera, objects);
}

bool Renderer2D::hasRendererTypeEnabled(RendererType flag)
{
    return (static_cast<uint32_t>(m_enabledRenderers) & static_cast<uint32_t>(flag)) != 0;
}

void Renderer2D::renderSprites(const CameraOld &camera, const std::vector<GameObject> &gameObjects)
{
    if (!m_batch)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    }

    generateIndexArrayForTriangle(m_indices);

    // UPDATE VERTICES
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

        // TODO: test ShaderEffectRequest rendering
        if (gameObject.hasComponent<ShaderEffectRequest>())
        {
            ShaderEffectRequest &effectRequest = gameObject.getComponent<ShaderEffectRequest>();

            GlResourceManager &resourceManager = GlResourceManager::instance();
            std::shared_ptr<Shader> shader =
                resourceManager.getShaderProgram(effectRequest.vertexShaderPath, effectRequest.fragmentShaderPath);
            m_activeShader = shader;

            // Set shader uniforms
            for (const auto &[uniformName, uniformValue] : effectRequest.uniforms)
            {
                std::visit(
                    [&](auto &&value)
                    {
                        using T = std::decay_t<decltype(value)>;
                        if constexpr (std::is_same_v<T, int>)
                        {
                            shader->setUniform1i(uniformName, value);
                        }
                        else if constexpr (std::is_same_v<T, float>)
                        {
                            shader->setUniform1f(uniformName, value);
                        }
                        else if constexpr (std::is_same_v<T, glm::vec4>)
                        {
                            shader->setUniform4f(uniformName, value);
                        }
                        else if constexpr (std::is_same_v<T, glm::mat4>)
                        {
                            shader->setUniform4fv(uniformName, value);
                        }
                    },
                    uniformValue);
            }
        }
    }

    if (m_vertices.empty())
    {
        return;
    }

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);

    size_t quadCount = m_vertices.size() / 4;
    size_t indexCount = quadCount * 6;

    m_batch->render(camera, indexCount, m_activeShader);
    m_activeShader = nullptr; // reset to default shader after rendering
}

void Renderer2D::renderSelection(const CameraOld &camera, const std::vector<GameObject> &selectedObjects)
{
    if (m_batch == nullptr)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    }

    // UPDATE VERTICES
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

        // We draw 4 thin quads (top, bottom, left, right) instead of
        // GL_LINES so the "lines" have a controllable thickness.
        const float thickness = 1.0f; // world units

        glm::vec4 selectionColor = glm::vec4(0.0f, 0.6f, 1.0f, 1.0f); // blue outline

        auto makeVertex = [&](const glm::vec3 &pos) -> Vertex
        {
            return Vertex{pos, selectionColor, glm::vec2(0.0f), -1};
        };

        // Top edge quad
        {
            topLeft.x -= thickness;
            topRight.x += thickness;
            glm::vec3 topLeftThick = {topLeft.x, topLeft.y + thickness, 0.0f};
            glm::vec3 topRightThick = {topRight.x, topRight.y + thickness, 0.0f};

            m_vertices.push_back(makeVertex(topLeftThick));
            m_vertices.push_back(makeVertex(topLeft));
            m_vertices.push_back(makeVertex(topRight));
            m_vertices.push_back(makeVertex(topRightThick));

            topLeft.x += thickness;
            topRight.x -= thickness;
        }

        // Bottom edge quad
        {
            bottomLeft.x -= thickness;
            bottomRight.x += thickness;
            glm::vec3 bottomLeftThick = {bottomLeft.x, bottomLeft.y - thickness, 0.0f};
            glm::vec3 bottomRightThick = {bottomRight.x, bottomRight.y - thickness, 0.0f};

            m_vertices.push_back(makeVertex(bottomLeft));
            m_vertices.push_back(makeVertex(bottomLeftThick));
            m_vertices.push_back(makeVertex(bottomRightThick));
            m_vertices.push_back(makeVertex(bottomRight));

            bottomLeft.x += thickness;
            bottomRight.x -= thickness;
        }

        // Left edge quad
        {
            glm::vec3 topLeftThick = {topLeft.x - thickness, topLeft.y, 0.0f};
            glm::vec3 bottomLeftThick = {bottomLeft.x - thickness, bottomLeft.y, 0.0f};

            m_vertices.push_back(makeVertex(topLeft));
            m_vertices.push_back(makeVertex(topLeftThick));
            m_vertices.push_back(makeVertex(bottomLeftThick));
            m_vertices.push_back(makeVertex(bottomLeft));
        }

        // Right edge quad
        {
            glm::vec3 topRightThick = {topRight.x + thickness, topRight.y, 0.0f};
            glm::vec3 bottomRightThick = {bottomRight.x + thickness, bottomRight.y, 0.0f};

            m_vertices.push_back(makeVertex(topRightThick));
            m_vertices.push_back(makeVertex(topRight));
            m_vertices.push_back(makeVertex(bottomRight));
            m_vertices.push_back(makeVertex(bottomRightThick));
        }
    }

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);

    size_t quadCount = m_vertices.size() / 4;
    size_t indexCount = quadCount * 8;

    m_batch->render(camera, indexCount);
}

void Renderer2D::setClearColor(glm::vec4 color)
{
    clearColor = color;
}

void Renderer2D::renderLayers(const CameraOld &camera, const std::vector<GameObject> &objects)
{
    for (RenderLayerType layer : RENDER_LAYER_ORDER)
    {
        m_finalFBO.bind();

        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (hasRendererTypeEnabled(RendererType::Grid))
        {
            // Keep grid tile size in sync with EditorMouseController.cpp.
            constexpr int GRID_TILE_SIZE = 16;
            m_gridRenderer.render(camera, GRID_TILE_SIZE);
        }

        if (hasRendererTypeEnabled(RendererType::Physics))
        {
            m_physicsRenderer.render(camera, objects);
        }

        if (hasRendererTypeEnabled(RendererType::Selection))
        {
            // Selection outlines and camera view bounds are both
            // editor-only overlays, grouped under the Selection flag.
            renderSelection(camera, objects);
            m_cameraBoundsRenderer.render(camera, objects);
        }

        if (hasRendererTypeEnabled(RendererType::Sprite))
        {
            renderSprites(camera, objects);
        }

        m_finalFBO.unbind();
    }
}
