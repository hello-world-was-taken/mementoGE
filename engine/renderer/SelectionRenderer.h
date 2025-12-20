#pragma once

#include "core/Camera.h"
#include "core/GameObject.h"

#include "opengl/Vertex.h"

#include "renderer/RenderBatch.h"

#include <functional>
#include <memory>
#include <vector>

class SelectionRenderer
{
public:
    SelectionRenderer();
    ~SelectionRenderer();
    SelectionRenderer(const SelectionRenderer &) = delete;
    SelectionRenderer &operator=(const SelectionRenderer &) = delete;

    void render(const CameraOld &camera, const std::vector<GameObject> &gameObjects);

private:
    void updateVertices(const std::vector<GameObject> &selectedObjects);

private:
    std::unique_ptr<RenderBatch> m_batch;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};
