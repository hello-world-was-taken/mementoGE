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
}

PhysicsRenderer::~PhysicsRenderer()
{
    std::cout << "PhysicsRenderer destructor called" << std::endl;
}

void PhysicsRenderer::render()
{

    if (!m_camera)
    {
        throw std::runtime_error("Attempted to render without setting camera!");
    }

    if (m_gameObjects == nullptr)
    {
        std::cout << "No gameobjects" << std::endl;
        return;
    }

    if (m_batch == nullptr)
        m_batch = std::make_unique<RenderBatch>(m_camera, m_indices, GL_LINES);

    updateVertices();

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);
    m_batch->render();
}

void PhysicsRenderer::updateVertices()
{
    m_vertices.clear();

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
            return Vertex{
                pos,
                borderColor,
                glm::vec2(0.0f), // no texture
                -1.0f            // sentinel tex index
            };
        };

        m_vertices.push_back(makeVertex(topLeft));
        m_vertices.push_back(makeVertex(bottomLeft));
        m_vertices.push_back(makeVertex(bottomRight));
        m_vertices.push_back(makeVertex(topRight));
    }

    m_batch->setVertexData(m_vertices);
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
    // m_indices.reserve(m_batch->BATCH_SIZE * m_batch->INDICES_PER_LINE_QUAD);
    // for (int i = 0; i < m_batch->BATCH_SIZE; i++)
    // {
    m_indices.resize(1000 * 8);
    for (int i = 0; i < 1000; i++)
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

void PhysicsRenderer::setCamera(std::shared_ptr<Camera> &camera)
{
    m_camera = camera;
}
