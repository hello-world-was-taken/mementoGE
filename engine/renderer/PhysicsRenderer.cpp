#include "core/components/BoxCollider2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"

#include "core/GLIncludes.h"

#include "opengl/Vertex.h"
#include "opengl/VertexAttribute.h"
#include "opengl/VertexBuffer.h"

#include "renderer/PhysicsRenderer.h"
#include "renderer/util.h"

#include "util/log_error.h"

#include <iostream>
#include <memory>

PhysicsRenderer::PhysicsRenderer()
{
    generateIndexArrayForLines(m_indices);
}

PhysicsRenderer::~PhysicsRenderer()
{
    std::cout << "PhysicsRenderer destructor called" << std::endl;
}

void PhysicsRenderer::render(const CameraOld &camera, const std::vector<GameObject> &gameObjects)
{

    if (m_batch == nullptr)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_LINES);
    }

    updateVertices(camera, gameObjects);

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);

    size_t quadCount = m_vertices.size() / 4;
    size_t indexCount = quadCount * 8;

    m_batch->render(camera, indexCount);
}

void PhysicsRenderer::updateSensor2DVertices(const GameObject &gameObject)
{
    Transform &transform = gameObject.getComponent<Transform>();
    const Sensor2D &sensor = gameObject.getComponent<Sensor2D>();

    float halfWidth = sensor.size.x * 0.5f;
    float halfHeight = sensor.size.y * 0.5f;

    glm::vec2 centerPos = glm::vec2{transform.position.x, transform.position.y} + sensor.offset;

    glm::vec3 bottomLeft = {centerPos.x - halfWidth, centerPos.y - halfHeight, 0.0f};
    glm::vec3 bottomRight = {centerPos.x + halfWidth, centerPos.y - halfHeight, 0.0f};
    glm::vec3 topRight = {centerPos.x + halfWidth, centerPos.y + halfHeight, 0.0f};
    glm::vec3 topLeft = {centerPos.x - halfWidth, centerPos.y + halfHeight, 0.0f};

    glm::vec4 borderColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // green

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

void PhysicsRenderer::updateBoxCollider2DVertices(const GameObject &gameObject)
{
    Transform &transform = gameObject.getComponent<Transform>();
    const BoxCollider2D &collider = gameObject.getComponent<BoxCollider2D>();

    float halfWidth = collider.size.x * 0.5f;
    float halfHeight = collider.size.y * 0.5f;

    glm::vec2 centerPos = glm::vec2{transform.position.x, transform.position.y} + collider.offset;

    glm::vec3 bottomLeft = {centerPos.x - halfWidth, centerPos.y - halfHeight, 0.0f};
    glm::vec3 bottomRight = {centerPos.x + halfWidth, centerPos.y - halfHeight, 0.0f};
    glm::vec3 topRight = {centerPos.x + halfWidth, centerPos.y + halfHeight, 0.0f};
    glm::vec3 topLeft = {centerPos.x - halfWidth, centerPos.y + halfHeight, 0.0f};

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

void PhysicsRenderer::updateVertices(const CameraOld &camera, const std::vector<GameObject> &gameObjects)
{
    m_vertices.clear();

    for (const GameObject &go : gameObjects)
    {
        if (go.hasComponent<BoxCollider2D>())
        {
            updateBoxCollider2DVertices(go);
        }

        if (go.hasComponent<Sensor2D>())
        {
            updateSensor2DVertices(go);
        }
    }

    m_batch->setVertexData(m_vertices);
}
