#pragma once

#include "core/GLIncludes.h"
#include "core/Camera.h"
#include "core/Camera.h"
#include "core/GameObject.h"
#include "core/Transform.h"

#include "opengl/Vertex.h"
#include "opengl/VertexArray.h"
#include "opengl/VertexBuffer.h"
#include "opengl/Indexbuffer.h"
#include "opengl/Shader.h"
#include "opengl/Texture.h"

#include "util/log_error.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>

// TODO: Once the engine adds support for Vulkan, common batch
// rendering logic needs to be abstracted.
class PhysicsRenderer
{
public:
    PhysicsRenderer();
    ~PhysicsRenderer();

    void render(std::shared_ptr<Camera> &camera);

    void updateVertexBuffer();
    void updateColliderLines();

    void generateVertexBuffer();
    void generateIndexArray();

    void setActiveGameObjects(std::vector<GameObject> *gameObjects);

private:
    std::vector<GameObject> *m_gameObjects;

    static const int BATCH_SIZE = 1000;    // 1000 QUADS
    static const int INDICES_PER_QUAD = 8; // 4 since we are drawing lines
    unsigned int m_indices[BATCH_SIZE * INDICES_PER_QUAD];

    // TODO: use smart pointer
    VertexArray *mp_vao;
    VertexBuffer *mp_vb;
    IndexBuffer *mp_ib;

    std::vector<int> m_texture_units = {0, 1, 2, 3, 4, 5, 6, 7};
    std::vector<Vertex> m_colliderLines;
};