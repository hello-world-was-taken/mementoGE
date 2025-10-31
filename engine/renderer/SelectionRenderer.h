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

    void render(const Camera &camera, const std::vector<std::reference_wrapper<GameObject>> &gameObjects);

private:
    void updateVertices(const std::vector<std::reference_wrapper<GameObject>> &selectedObjects);
    void generateIndexArray();

private:
    std::unique_ptr<RenderBatch> m_batch;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};
