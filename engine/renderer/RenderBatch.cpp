#include "core/components/Sprite.h"

#include "core/GLIncludes.h"
#include "core/GlResourceManager.h"
#include "core/Window.h"

#include "opengl/Vertex.h"
#include "opengl/VertexAttribute.h"
#include "opengl/VertexBuffer.h"

#include "renderer/Constants.h"
#include "renderer/RenderBatch.h"

#include "util/LogChannels.h"
#include "util/PathUtils.h"
#include "util/log_error.h"

#include <memory>
#include <sawyer/Logger.h>

RenderBatch::RenderBatch(std::vector<unsigned int> indices, GLenum drawMode) : m_indices(indices), m_drawMode(drawMode)
{
    setupBuffers();
}

RenderBatch::~RenderBatch()
{
    LOG_DEBUG(LogChannel::Renderer, "RenderBatch destructor called");

    m_vbo->unbind();
    m_ibo->unbind();
    m_vao->unbind();
}

void RenderBatch::setupBuffers()
{
    // TODO: We may not use the total space.
    unsigned int bufferSize = BATCH_SIZE * VERTEX_PER_QUAD * sizeof(Vertex);

    m_vao = std::make_unique<VertexArray>();
    m_vao->bind();

    m_vbo = std::make_unique<VertexBuffer>(bufferSize, GL_DYNAMIC_DRAW);
    m_vbo->bind();

    m_vao->attachVertexAttribute({3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0});
    m_vao->attachVertexAttribute({4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color)});
    m_vao->attachVertexAttribute({2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texture)});
    m_vao->attachVertexAttribute({1, GL_INT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texSlot)});

    if (m_drawMode == GL_TRIANGLES)
    {
        // TODO: FIX INDICES SIZE MISMATCH
        m_ibo = std::make_unique<IndexBuffer>(BATCH_SIZE * INDICES_PER_QUAD, GL_STATIC_DRAW);
    }
    else if (m_drawMode == GL_LINES)
    {
        m_ibo = std::make_unique<IndexBuffer>(BATCH_SIZE * INDICES_PER_LINE_QUAD, GL_STATIC_DRAW);
    }
    m_ibo->bind();

    // Once we associate vbo and ibo with the vao, we should only unbound the vao
    // to not break the association.
    m_vao->unbind();
}

void RenderBatch::setVertexData(std::vector<Vertex> &vertices)
{
    m_vertices.clear();
    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());

    m_vbo->updateBufferData(m_vertices);
}

void RenderBatch::setIndexData(std::vector<unsigned int> &indices)
{
    m_indices.clear();
    m_indices.insert(m_indices.end(), indices.begin(), indices.end());

    m_ibo->updateIndicesData(m_indices.data(), m_indices.size());
}

void RenderBatch::render(const CameraOld &camera,
    int drawCount,
    std::shared_ptr<Shader> customShader,
    const std::vector<std::shared_ptr<Texture>> &textures)
{
    auto vertexShaderPath = getEngineAssetPath("shader/vertex.shader");
    auto fragmentShaderPath = getEngineAssetPath("shader/fragment.shader");

    std::shared_ptr<Shader> shader =
        customShader ? customShader
                     : GlResourceManager::instance().getShaderProgram(vertexShaderPath, fragmentShaderPath);

    shader->use();
    shader->setUniform4fv("u_view_matrix", camera.getViewMatrix());
    shader->setUniform4fv("u_projection_matrix", camera.getProjectionMatrix());
    // Custom effects can animate in both the editor viewport and gameplay,
    // without depending on a scene system to feed them a clock.
    shader->setUniform1f("u_GlobalTime", Window::getWindowTime());
    shader->setMultipleTextureUnits("textures", m_textureUnits.data(), m_textureUnits.size());

    for (size_t slot = 0; slot < textures.size(); ++slot)
    {
        textures[slot]->bind(static_cast<unsigned int>(slot));
    }

    m_vao->bind();
    glClearError();
    if (m_drawMode == GL_TRIANGLES)
    {
        glDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, nullptr);
        glCheckError("glDrawTriangles", __FILE__, __LINE__);
    }
    else if (m_drawMode == GL_LINES)
    {
        glDrawElements(GL_LINES, drawCount, GL_UNSIGNED_INT, nullptr);
        glCheckError("glDrawLines", __FILE__, __LINE__);
    }
    m_vao->unbind();
}
