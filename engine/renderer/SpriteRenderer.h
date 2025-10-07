#pragma once

#include "core/Camera.h"
#include "core/GameObject.h"

#include "opengl/Vertex.h"

#include "renderer/RenderBatch.h"

#include <memory>

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    void render(const Camera &camera, const std::vector<GameObject> &gameObjects);
    void updateVertices(const Camera &camera, const std::vector<GameObject> &gameObjects);
    void generateIndexArray();

private:
    std::unique_ptr<RenderBatch> m_batch;

    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};