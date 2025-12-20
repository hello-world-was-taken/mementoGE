#pragma once

#include "core/Camera.h"
#include "core/GLIncludes.h"
#include "core/GameObject.h"

#include "opengl/Vertex.h"

#include "renderer/RenderBatch.h"

#include "util/log_error.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class PhysicsRenderer
{
public:
    PhysicsRenderer();
    ~PhysicsRenderer();
    PhysicsRenderer(const PhysicsRenderer &) = delete;
    PhysicsRenderer &operator=(const PhysicsRenderer &) = delete;

    void render(const CameraOld &camera, const std::vector<GameObject> &gameObjects);

private:
    void updateBoxCollider2DVertices(const GameObject &gameObject);
    void updateSensor2DVertices(const GameObject &gameObject);
    void updateVertices(const CameraOld &camera, const std::vector<GameObject> &gameObjects);

private:
    std::unique_ptr<RenderBatch> m_batch;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};