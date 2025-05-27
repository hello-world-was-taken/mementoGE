#include "core/GLIncludes.h"
#include "core/Resource.h"
#include "core/Sprite.h"

#include "opengl/Vertex.h"
#include "opengl/VertexBuffer.h"
#include "opengl/VertexAttribute.h"

#include "physics/BoxCollider2D.h"

#include "renderer/PhysicsRenderer.h"
#include "util/log_error.h"

#include <iostream>
#include <memory>

PhysicsRenderer::PhysicsRenderer()
{
    generateIndexArray();
    generateVertexBuffer();
}

PhysicsRenderer::~PhysicsRenderer()
{
    std::cout << "PhysicsRenderer destructor called" << std::endl;

    mp_vb->unbind();
    mp_ib->unbind();
    mp_vao->unbind();

    delete mp_vb;
    delete mp_ib;
    delete mp_vao;
}

void PhysicsRenderer::render(std::shared_ptr<Camera> &camera)
{
    if (!m_gameObjects)
        return;

    mp_vao->bind();
    mp_vb->bind();

    // repopulate the vertices with the new data
    updateColliderLines();

    std::shared_ptr<Shader>
        shader = Resource::getShaderProgram("../assets/shader/vertex.shader", "../assets/shader/fragment.shader");
    shader.get()->use();

    // set the view matrix
    glm::mat4 u_view_matrix = camera->getViewMatrix();
    shader.get()->setUniform4fv("u_view_matrix", u_view_matrix);

    // set the projection matrix
    glm::mat4 u_projection_matrix = camera->getProjectionMatrix();
    shader.get()->setUniform4fv("u_projection_matrix", u_projection_matrix);

    shader.get()->setMultipleTextureUnits("textures", m_texture_units.data(), m_texture_units.size());

    glClearError();
    glDrawElements(GL_LINES, BATCH_SIZE * INDICES_PER_QUAD, GL_UNSIGNED_INT, nullptr);
    glCheckError("glDrawLines", __FILE__, __LINE__);

    mp_vb->unbind();
    mp_vao->unbind();
}

void PhysicsRenderer::updateColliderLines()
{
    m_colliderLines.clear();

    for (GameObject &gameObject : *m_gameObjects)
    {
        if (!gameObject.hasComponent<BoxCollider2D>())
            continue;

        Transform &transform = gameObject.getComponent<Transform>();
        const BoxCollider2D &collider = gameObject.getComponent<BoxCollider2D>();

        float width = collider.m_size.x;
        float height = collider.m_size.y;
        glm::vec2 bottomLeftPos = glm::vec2{transform.getPosition()->x, transform.getPosition()->y} + collider.m_offset;

        glm::vec3 bottomLeft = {bottomLeftPos.x, bottomLeftPos.y, 0.0f};
        glm::vec3 bottomRight = {bottomLeftPos.x + width, bottomLeftPos.y, 0.0f};
        glm::vec3 topRight = {bottomLeftPos.x + width, bottomLeftPos.y + height, 0.0f};
        glm::vec3 topLeft = {bottomLeftPos.x, bottomLeftPos.y + height, 0.0f};

        glm::vec4 borderColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // red

        auto makeVertex = [&](const glm::vec3 &pos) -> Vertex
        {
            std::cout << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z << std::endl;
            return Vertex{
                pos,
                borderColor,
                glm::vec2(0.0f), // no texture
                -1.0f            // sentinel tex index
            };
        };

        m_colliderLines.push_back(makeVertex(topLeft));
        m_colliderLines.push_back(makeVertex(bottomLeft));
        m_colliderLines.push_back(makeVertex(bottomRight));
        m_colliderLines.push_back(makeVertex(topRight));
    }

    // update the vertex buffer
    mp_vb->updateBufferData(m_colliderLines);
}

void PhysicsRenderer::generateVertexBuffer()
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

/**
 * @brief Generates indices to draw box collider wireframes as line segments.
 *
 * Each box collider has 4 vertices added in this order:
 *   0: topLeft, 1: bottomLeft, 2: bottomRight, 3: topRight
 *
 * To form the rectangle edges, we add 4 lines:
 *   - topLeft → bottomLeft
 *   - bottomLeft → bottomRight
 *   - bottomRight → topRight
 *   - topRight → topLeft
 *
 * Each box contributes 8 indices (4 lines * 2 vertices).
 */
void PhysicsRenderer::generateIndexArray()
{
    for (int i = 0; i < BATCH_SIZE; i++)
    {
        // topLeft -> bottomLeft
        m_indices[i * 8] = i * 4;
        m_indices[i * 8 + 1] = i * 4 + 1;

        // bottomLeft -> bottomRight
        m_indices[i * 8 + 2] = i * 4 + 1;
        m_indices[i * 8 + 3] = i * 4 + 2;

        // bottomRight -> topRight
        m_indices[i * 8 + 4] = i * 4 + 2;
        m_indices[i * 8 + 5] = i * 4 + 3;

        // topRight -> topLeft
        m_indices[i * 8 + 6] = i * 4 + 3;
        m_indices[i * 8 + 7] = i * 4;
    }

}

void PhysicsRenderer::setActiveGameObjects(std::vector<GameObject> *gameObjects)
{
    m_gameObjects = gameObjects;
}