#include "core/GLIncludes.h"
#include "core/Resource.h"
#include "core/Sprite.h"

#include "opengl/Vertex.h"
#include "opengl/VertexBuffer.h"
#include "opengl/VertexAttribute.h"

#include "renderer/RenderBatch.h"
#include "util/log_error.h"

#include <iostream>
#include <memory>

RenderBatch::RenderBatch(
    std::shared_ptr<Camera> camera,
    std::vector<unsigned int> indices,
    GLenum drawMode)
    : m_camera(camera),
      m_indices(indices),
      m_drawMode(drawMode)
{
    setupBuffers();
}

RenderBatch::~RenderBatch()
{
    std::cout << "RenderBatch destructor called" << std::endl;

    m_vbo->unbind();
    m_ibo->unbind();
    m_vao->unbind();
}

void RenderBatch::setupBuffers()
{
    // TODO: We may not use the total space.
    unsigned int bufferSize = 1000 * 4 * sizeof(Vertex);

    m_vao = std::make_unique<VertexArray>();
    m_vao->bind();

    m_vbo = std::make_unique<VertexBuffer>(bufferSize, GL_DYNAMIC_DRAW);
    m_vbo->bind();

    m_vao->attachVertexAttribute({3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0});
    m_vao->attachVertexAttribute({4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color)});
    m_vao->attachVertexAttribute({2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texture)});
    m_vao->attachVertexAttribute({1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texIndex)});

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

void RenderBatch::setVertexData(std::vector<Vertex> vertices)
{
    m_vertices.clear();
    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());

    m_vbo->updateBufferData(m_vertices);
}

void RenderBatch::setIndexData(std::vector<unsigned int> indices)
{
    m_indices.clear();
    m_indices.insert(m_indices.end(), indices.begin(), indices.end());

    m_ibo->updateIndicesData(m_indices.data(), m_indices.size());
}

void RenderBatch::render(std::shared_ptr<Shader> customShader)
{
    std::shared_ptr<Shader> shader = customShader ? customShader
                                                  : Resource::getShaderProgram("../assets/shader/vertex.shader", "../assets/shader/fragment.shader");

    shader->use();
    shader->setUniform4fv("u_view_matrix", m_camera->getViewMatrix());
    shader->setUniform4fv("u_projection_matrix", m_camera->getProjectionMatrix());
    shader->setMultipleTextureUnits("textures", m_textureUnits.data(), m_textureUnits.size());

    m_vao->bind();
    glClearError();
    if (m_drawMode == GL_TRIANGLES)
    {
        glDrawElements(GL_TRIANGLES, BATCH_SIZE * INDICES_PER_QUAD, GL_UNSIGNED_INT, nullptr);
        glCheckError("glDrawTriangles", __FILE__, __LINE__);
    }
    else if (m_drawMode == GL_LINES)
    {
        glDrawElements(GL_LINES, BATCH_SIZE * INDICES_PER_LINE_QUAD, GL_UNSIGNED_INT, nullptr);
        glCheckError("glDrawLines", __FILE__, __LINE__);
    }
    m_vao->unbind();
}
