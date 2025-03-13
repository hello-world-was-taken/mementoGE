#pragma once

#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>

#include "util/log_error.h"
#include "opengl/Vertex.h"
#include "opengl/VertexArray.h"
#include "opengl/VertexBuffer.h"
#include "opengl/Indexbuffer.h"
#include "opengl/Shader.h"
#include "opengl/Texture.h"
#include "core/Camera.h"
#include "core/Camera.h"
#include "core/GameObject.h"
#include "core/Transform.h"

// TODO: Once the engine adds support for Vulkan, common batch
// rendering logic needs to be abstracted.
class RenderBatch
{
public:
    RenderBatch(const std::shared_ptr<Camera> camera, std::vector<GameObject>& m_gameObjects);
    ~RenderBatch();

    void render();
    // void getQuad();
    // void generateVertexArray();
    void updateVertexBuffer();
    void generateVertexBuffer();
    void generateIndexArray();
    std::vector<glm::vec3> transformQuad(glm::mat4x4 transformMatrix, std::vector<glm::vec3> quad);

private:
    const std::shared_ptr<Camera> m_camera;
    std::vector<GameObject>& m_gameObjects;
    std::vector<Vertex> m_vertices;
    static const int BATCH_SIZE = 1000;    // 1000 QUADS
    static const int INDICES_PER_QUAD = 6; // 6 indices per quad
    unsigned int m_indices[BATCH_SIZE * INDICES_PER_QUAD];
    // TODO: use smart pointer
    VertexArray *mp_vao;
    VertexBuffer *mp_vb;
    IndexBuffer *mp_ib;

    std::vector<int> m_texture_units = {0, 1, 2, 3, 4, 5, 6, 7};
};