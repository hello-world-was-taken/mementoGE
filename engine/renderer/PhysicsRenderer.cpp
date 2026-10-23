#include "core/components/BoxCollider2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"

#include "core/GLIncludes.h"

#include "opengl/Vertex.h"
#include "opengl/VertexAttribute.h"
#include "opengl/VertexBuffer.h"

#include "physics/CollisionShapeTransform.h"
#include "renderer/PhysicsRenderer.h"
#include "renderer/util.h"

#include "editor/Constants.h"
#include "util/LogChannels.h"
#include "util/log_error.h"

#include <cmath>
#include <memory>
#include <sawyer/Logger.h>

namespace
{
glm::vec2 rotateVector(const glm::vec2 &value, float rotation)
{
    const float cosine = std::cos(rotation);
    const float sine = std::sin(rotation);
    return {value.x * cosine - value.y * sine, value.x * sine + value.y * cosine};
}

void appendRectangleOutline(std::vector<Vertex> &vertices,
    const glm::vec2 &center,
    const glm::vec2 &halfSize,
    const glm::vec4 &color,
    float thickness,
    float rotation = 0.0f)
{
    const float left = center.x - halfSize.x;
    const float right = center.x + halfSize.x;
    const float bottom = center.y - halfSize.y;
    const float top = center.y + halfSize.y;

    auto appendQuad = [&](float quadLeft, float quadBottom, float quadRight, float quadTop)
    {
        const float cosine = std::cos(rotation);
        const float sine = std::sin(rotation);
        auto rotateAroundCenter = [&](const glm::vec2 &point)
        {
            const glm::vec2 local = point - center;
            return center + glm::vec2{local.x * cosine - local.y * sine, local.x * sine + local.y * cosine};
        };
        auto appendVertex = [&](const glm::vec2 &point)
        {
            const glm::vec2 rotated = rotateAroundCenter(point);
            vertices.emplace_back(Vertex{{rotated.x, rotated.y, 0.0f}, color, {0.0f, 0.0f}, -1});
        };

        appendVertex({quadLeft, quadTop});
        appendVertex({quadLeft, quadBottom});
        appendVertex({quadRight, quadBottom});
        appendVertex({quadRight, quadTop});
    };

    appendQuad(left, top - thickness, right, top);
    appendQuad(left, bottom, right, bottom + thickness);
    appendQuad(left, bottom, left + thickness, top);
    appendQuad(right - thickness, bottom, right, top);
}
} // namespace

PhysicsRenderer::PhysicsRenderer()
{
    generateIndexArrayForTriangle(m_indices);
}

PhysicsRenderer::~PhysicsRenderer()
{
    LOG_DEBUG(LogChannel::Renderer, "PhysicsRenderer destructor called");
}

void PhysicsRenderer::render(const Camera &camera, double elapsedTime, const std::vector<GameObject> &gameObjects)
{

    if (m_batch == nullptr)
    {
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);
    }

    updateVertices(camera, gameObjects);

    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);

    size_t quadCount = m_vertices.size() / 4;
    size_t indexCount = quadCount * INDICES_PER_QUAD;

    m_batch->render(camera, elapsedTime, indexCount);
}

void PhysicsRenderer::updateSensor2DVertices(const GameObject &gameObject, float lineThickness)
{
    const Transform &transform = gameObject.getComponent<Transform>();
    const Sensor2D &sensor = gameObject.getComponent<Sensor2D>();
    const Sprite *sprite = gameObject.hasComponent<Sprite>() ? &gameObject.getComponent<Sprite>() : nullptr;
    const bool flipX = sprite && sprite->flipX;
    const bool flipY = sprite && sprite->flipY;
    const glm::vec4 borderColor{0.384f, 0.659f, 0.451f, 1.0f}; // softened sensor green
    const float bodyRotation = glm::radians(transform.rotation.z);

    for (const auto &shape : sensor.shapes)
    {
        if (!sensor.isActive || !shape.animatedActive)
        {
            continue;
        }
        const glm::vec2 offset = mirrorCollisionOffset(shape.animatedOffset, flipX, flipY);
        glm::vec2 center = glm::vec2{transform.position.x, transform.position.y} + rotateVector(offset, bodyRotation);
        appendRectangleOutline(m_vertices, center, shape.animatedSize * 0.5f, borderColor, lineThickness, bodyRotation);
    }
}

void PhysicsRenderer::updateBoxCollider2DVertices(const GameObject &gameObject, float lineThickness)
{
    const Transform &transform = gameObject.getComponent<Transform>();
    const BoxCollider2D &collider = gameObject.getComponent<BoxCollider2D>();
    const Sprite *sprite = gameObject.hasComponent<Sprite>() ? &gameObject.getComponent<Sprite>() : nullptr;
    const bool flipX = sprite && sprite->flipX != collider.referenceFlipX;
    const bool flipY = sprite && sprite->flipY != collider.referenceFlipY;
    const glm::vec4 borderColor{0.780f, 0.373f, 0.373f, 1.0f}; // softened collider red
    const float bodyRotation = glm::radians(transform.rotation.z);

    for (const auto &shape : collider.shapes)
    {
        if (!shape.animatedActive)
        {
            continue;
        }
        const glm::vec2 offset = mirrorCollisionOffset(shape.animatedOffset, flipX, flipY);
        const float rotation = mirrorCollisionRotation(shape.animatedRotation, flipX, flipY);
        glm::vec2 center = glm::vec2{transform.position.x, transform.position.y} + rotateVector(offset, bodyRotation);
        appendRectangleOutline(m_vertices,
            center,
            shape.animatedSize * 0.5f,
            borderColor,
            lineThickness,
            bodyRotation + glm::radians(rotation));
    }
}

void PhysicsRenderer::updateVertices(const Camera &camera, const std::vector<GameObject> &gameObjects)
{
    m_vertices.clear();
    const float lineThickness = (LOGICAL_WIDTH / WINDOW_WIDTH) * camera.zoom;

    for (const GameObject &go : gameObjects)
    {
        if (go.hasComponent<BoxCollider2D>())
        {
            updateBoxCollider2DVertices(go, lineThickness);
        }

        if (go.hasComponent<Sensor2D>())
        {
            updateSensor2DVertices(go, lineThickness);
        }
    }

    m_batch->setVertexData(m_vertices);
}
