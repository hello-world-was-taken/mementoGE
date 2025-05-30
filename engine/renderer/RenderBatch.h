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
class RenderBatch
{
public:
    RenderBatch(std::shared_ptr<Camera> camera, std::vector<unsigned int> indices, GLenum drawMode);
    ~RenderBatch();

    void setVertexData(std::vector<Vertex> vertices);
    void setIndexData(std::vector<unsigned int> indices);
    void render(std::shared_ptr<Shader> customShader = nullptr);

private:
    void setupBuffers();

public:
    const int BATCH_SIZE = 1000;
    const int VERTEX_PER_QUAD = 4;
    const int INDICES_PER_QUAD = 6;
    const int INDICES_PER_LINE_QUAD = 8;

private:
    GLenum m_drawMode;

    std::shared_ptr<Camera> m_camera;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;

    std::unique_ptr<VertexArray> m_vao;
    std::unique_ptr<VertexBuffer> m_vbo;
    std::unique_ptr<IndexBuffer> m_ibo;

    std::vector<int> m_textureUnits = {0, 1, 2, 3, 4, 5, 6, 7};
};