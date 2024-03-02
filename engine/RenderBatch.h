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

class RenderBatch
{
private:
    Camera* m_camera;
    std::vector<Vertex> vertices;
    static const int BATCH_SIZE = 1000;  // 1000 QUADS
    static const int INDICES_PER_QUAD = 6;  // 6 indices per quad
    unsigned int indices[BATCH_SIZE * INDICES_PER_QUAD];
    unsigned int vao;
    VertexBuffer vb;

public:
    RenderBatch(Camera *camera);
    ~RenderBatch();

    void render();
    void addVertex(Vertex vertex);
    // void generateVertexArray();
    void generateVertexBuffer();
    void generateIndexArray();
};