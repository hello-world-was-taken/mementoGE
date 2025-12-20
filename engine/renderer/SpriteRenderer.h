#pragma once

#include "core/components/RenderLayer.h"

#include "core/Camera.h"
#include "core/GameObject.h"

#include "opengl/Vertex.h"

#include "renderer/RenderBatch.h"

#include <map>
#include <memory>

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    void render(const CameraOld &camera, const std::vector<GameObject> &gameObjects);

private:
    void updateVertices(const std::vector<GameObject> &gameObjects);

private:
    std::unique_ptr<RenderBatch> m_batch;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};