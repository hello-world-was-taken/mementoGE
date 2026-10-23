#pragma once

#include "core/components/RenderLayer.h"

#include "core/components/Camera.h"
#include "core/GameObject.h"

#include "opengl/Vertex.h"
#include "opengl/Shader.h"

#include "renderer/RenderBatch.h"

#include <map>
#include <memory>

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    // Text geometry is rebuilt here on the render owner, so objects are mutable.
    void render(const Camera &camera, double elapsedTime, std::vector<GameObject> &gameObjects);

private:
    std::unique_ptr<RenderBatch> m_batch;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::shared_ptr<Shader> m_currentShader = nullptr;
};
