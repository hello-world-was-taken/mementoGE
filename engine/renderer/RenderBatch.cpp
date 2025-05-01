#include <iostream>
#include <GL/glew.h>
#include <memory>

#include "opengl/Vertex.h"
#include "opengl/VertexBuffer.h"
#include "opengl/VertexAttribute.h"
#include "renderer/RenderBatch.h"
#include "core/Resource.h"
#include "core/Sprite.h"
#include "util/log_error.h"

RenderBatch::RenderBatch(
    const std::shared_ptr<Camera> camera,
    std::vector<GameObject> &gameObjects)
    : m_camera(camera), m_gameObjects(gameObjects)
{
    generateIndexArray();
    generateVertexBuffer();
}

RenderBatch::~RenderBatch()
{
    std::cout << "RenderBatch destructor called" << std::endl;

    mp_vb->unbind();
    mp_ib->unbind();
    mp_vao->unbind();

    delete mp_vb;
    delete mp_ib;
    delete mp_vao;
}

void RenderBatch::render()
{
    mp_vao->bind();

    // repopulate the vertices with the new data
    updateVertexBuffer();

    std::shared_ptr<Shader> shader = Resource::getShaderProgram("../assets/shader/vertex.shader", "../assets/shader/fragment.shader");
    shader.get()->use();

    // set the view matrix
    glm::mat4 u_view_matrix = m_camera->getViewMatrix();
    shader.get()->setUniform4fv("u_view_matrix", u_view_matrix);

    // set the projection matrix
    glm::mat4 u_projection_matrix = m_camera->getProjectionMatrix();
    shader.get()->setUniform4fv("u_projection_matrix", u_projection_matrix);

    shader.get()->setMultipleTextureUnits("textures", m_texture_units.data(), m_texture_units.size());

    glClearError();
    glDrawElements(GL_TRIANGLES, BATCH_SIZE * INDICES_PER_QUAD, GL_UNSIGNED_INT, nullptr);
    glCheckError("glDrawTriangles", __FILE__, __LINE__);

    mp_vao->unbind();
}

void RenderBatch::updateVertexBuffer()
{
    // clear the vertices vector
    m_vertices.clear();

    for (GameObject &gameObject : m_gameObjects)
    {

        Transform transform = gameObject.getComponent<Transform>();

        // The world coordinate is model matrix * local quad.
        std::vector<glm::vec3> transformedQuad = gameObject.getWorldCoordinateQuad();

        if (gameObject.hasComponent<Sprite>())
        {
            Sprite sprite = gameObject.getComponent<Sprite>();

            for (int i = 0; i < transformedQuad.size(); i++)
            {
                m_vertices.push_back({transformedQuad[i],
                                      sprite.getColor(),
                                      sprite.getTextureCoordinates()[i], // TODO: do we need to retrieve this from the sprite renderer?
                                      (float)sprite.getTexture()->getTextureUnit()});
            }
        }
    }

    // update the vertex buffer
    mp_vb->updateBufferData(m_vertices);
}

// TODO: why is this function creating an index buffer?
void RenderBatch::generateVertexBuffer()
{
    // 1000 Quads * 4 vertices per quad * sizeof(Vertex)
    unsigned int bufferSize = 1000 * 4 * sizeof(Vertex);
    glClearError();

    mp_vao = new VertexArray();
    mp_vao->bind();

    mp_vb = new VertexBuffer(bufferSize, GL_DYNAMIC_DRAW);
    mp_vb->bind();

    mp_vao->attachVertexAttribute(VertexAttribute{3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0});
    mp_vao->attachVertexAttribute(VertexAttribute{4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color)});
    mp_vao->attachVertexAttribute(VertexAttribute{2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texture)});
    mp_vao->attachVertexAttribute(VertexAttribute{1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texIndex)});

    mp_ib = new IndexBuffer{m_indices, BATCH_SIZE * INDICES_PER_QUAD, GL_STATIC_DRAW};
    mp_ib->bind();

    // Once we associate vbo and ibo with the vao, we should only unbound the vao
    // to not break the association.
    mp_vao->unbind();
}

void RenderBatch::generateIndexArray()
{
    // The pattern of the indices is the same for each quad
    // 0, 1, 2, 0, 3, 2     // then by adding 4 to each index
    // +4,+4,+4,+4,+4,+4
    // 4, 5, 6, 4, 7, 6     // we get this one and so on.
    // The for-loop below generates the indices for each quad

    for (int i = 0; i < BATCH_SIZE; i++)
    {
        m_indices[i * 6] = i * 4;
        m_indices[i * 6 + 1] = i * 4 + 1;
        m_indices[i * 6 + 2] = i * 4 + 2;
        m_indices[i * 6 + 3] = i * 4;
        m_indices[i * 6 + 4] = i * 4 + 3;
        m_indices[i * 6 + 5] = i * 4 + 2;
    }
}