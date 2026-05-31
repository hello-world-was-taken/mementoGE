#pragma once

#include "core/Camera.h"
#include "core/GameObject.h"
#include "opengl/Vertex.h"
#include "renderer/RenderBatch.h"

#include <memory>
#include <vector>

class GizmoRenderer
{
public:
    GizmoRenderer();
    ~GizmoRenderer();

    GizmoRenderer(const GizmoRenderer &) = delete;
    GizmoRenderer &operator=(const GizmoRenderer &) = delete;

    void render(const CameraOld &camera, double elapsedTime, const std::vector<GameObject> &selectedObjects);

private:
    void updateVertices(const CameraOld &camera, const std::vector<GameObject> &selectedObjects);
    void appendQuad(glm::vec2 min, glm::vec2 max, const glm::vec4 &color);
    void appendTriangle(glm::vec2 first, glm::vec2 second, glm::vec2 third, const glm::vec4 &color);

    std::unique_ptr<RenderBatch> m_batch;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};
