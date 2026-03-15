#pragma once

#include "core/Camera.h"
#include "core/GameObject.h"

#include "opengl/Vertex.h"

#include "renderer/RenderBatch.h"

#include <memory>
#include <vector>

class CameraBoundsRenderer
{
public:
    CameraBoundsRenderer();
    ~CameraBoundsRenderer();
    CameraBoundsRenderer(const CameraBoundsRenderer &) = delete;
    CameraBoundsRenderer &operator=(const CameraBoundsRenderer &) = delete;

    // Draw a thin box representing the gameplay camera's view.
    // For now this is a fixed box at the origin using the logical
    // resolution, rendered with the editor camera.
    void render(const CameraOld &editorCamera, const std::vector<GameObject> &gameObjects);

private:
    void updateVertices(const std::vector<GameObject> &gameObjects);

private:
    std::unique_ptr<RenderBatch> m_batch;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};
