#pragma once

#include "core/GLIncludes.h"
#include "core/Camera.h"
#include "core/GameObject.h"

#include "opengl/Vertex.h"

#include "renderer/RenderBatch.h"

#include "util/log_error.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>

class PhysicsRenderer
{
public:
    PhysicsRenderer();
    ~PhysicsRenderer();

    void render(
        const Camera &camera,
        const std::vector<GameObject> &gameObjects);

private:
    void updateVertices(
        const Camera &camera,
        const std::vector<GameObject> &gameObjects);
    void generateIndexArray();

private:
    std::unique_ptr<RenderBatch> m_batch;
    std::vector<GameObject> *m_gameObjects = nullptr;

    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};