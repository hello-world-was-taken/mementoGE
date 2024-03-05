#pragma once

#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"

#include "engine/Vertex.h"
#include "engine/VertexBuffer.h"
#include "engine/Indexbuffer.h"
#include "util/log_error.h"
#include "engine/Shader.h"
#include "engine/Camera.h"
#include "engine/Texture.h"
#include "engine/Camera.h"
#include "engine/SpriteRenderer.h"
#include "engine/GameObject.h"
#include "engine/Transform.h"

class RenderBatch
{
private:
    const Camera *m_camera;
    std::shared_ptr<std::vector<std::shared_ptr<GameObject>>> m_gameObjects;
    std::vector<Vertex> vertices;
    static const int BATCH_SIZE = 1000;    // 1000 QUADS
    static const int INDICES_PER_QUAD = 6; // 6 indices per quad
    unsigned int indices[BATCH_SIZE * INDICES_PER_QUAD];
    unsigned int vao;
    VertexBuffer vb;
    std::vector<glm::vec3> m_quad = {
        {0.0f, 100.0f, 0.0f},    // top left
        {0.0f, 0.0f, 0.0f},      // bottom left
        {100.0f, 0.0f, 0.0f},    // bottom right
        {100.0f, 100.0f, 0.0f}}; // top right

    std::vector<int> m_texture_units = {0, 1, 2, 3, 4, 5, 6, 7};

public:
    RenderBatch(const Camera *camera, std::shared_ptr<std::vector<std::shared_ptr<GameObject>>> m_gameObjects);
    ~RenderBatch();

    void render();
    // void getQuad();
    // void generateVertexArray();
    void updateVertexBuffer();
    void generateVertexBuffer();
    void generateIndexArray();
    std::vector<glm::vec3> transformQuad(glm::mat4x4 transformMatrix);
};