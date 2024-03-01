#include "engine/RenderBatch.h"
#include "game/Scene.h"
#include <iostream>
#include "util/log_error.h"
#include <GL/glew.h>

RenderBatch::RenderBatch(Scene *scene) : m_scene(scene){
    generateIndexArray();
    generateVertexBuffer();
}

RenderBatch::~RenderBatch() {
    glDeleteVertexArrays(1, &this->vao);
}

void RenderBatch::render() {
    // rendere gets called every frame
    std::vector<Vertex> vertices = {
        {{0.0f, 100.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f},   // top left
        {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0.0f},     // bottom left
        {{100.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0.0f},   // bottom right
        {{100.0f, 100.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f}, // top right

        {{110.0f, 100.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f}, // top left
        {{110.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0.0f},   // bottom left
        {{210.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0.0f},   // bottom right
        {{210.0f, 100.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f}  // top right
    };

    glBindVertexArray(vao); // Bind VAO

    vb.bind();
    vb.updateBufferData(vertices);

    glClear(GL_COLOR_BUFFER_BIT);

    Shader shader("../assets/shader/vertex.shader", "../assets/shader/fragment.shader");
    shader.use();

    // set the view matrix
    glm::mat4 u_view_matrix = m_scene->getCamera()->getViewMatrix();
    shader.setUniform4fv("u_view_matrix", u_view_matrix);

    // set the projection matrix
    glm::mat4 u_projection_matrix = m_scene->getCamera()->getProjectionMatrix();
    shader.setUniform4fv("u_projection_matrix", u_projection_matrix);

    // set the model matrix
    glm::mat4 u_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    shader.setUniform4fv("u_model_matrix", u_model_matrix);


    // use uniform texture
    const char *texture_path = "../assets/texture/slice01_01.png";
    Texture texture(texture_path);
    texture.bind();

    shader.setUniform1i("our_texture", 0);

    glClearError();
    glDrawElements(GL_TRIANGLES, BATCH_SIZE * INDICES_PER_QUAD, GL_UNSIGNED_INT, nullptr);
    glCheckError("glDrawArrays", __FILE__, __LINE__);

    glBindVertexArray(0); // Unbind VAO
}

void RenderBatch::generateVertexBuffer() {
    // 1000 Quads * 4 vertices per quad * sizeof(Vertex)
    unsigned int bufferSize = 1000 * 4 * sizeof(Vertex);
    glClearError();
    if (glGenVertexArrays == NULL)
    {
        std::cout << "Something is wrong" << &vao << std::endl;
    }
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    vb = VertexBuffer(bufferSize, GL_DYNAMIC_DRAW);

    vb.addAttribute(VertexAttribute(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0));
    vb.addAttribute(VertexAttribute(4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color)));
    vb.addAttribute(VertexAttribute(2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texture)));
    vb.addAttribute(VertexAttribute(1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texIndex)));

    // Setting the vertex attribute pointers adds the VBO to the currently bound VAO
    vb.setAttributePointers();

    IndexBuffer ib(indices, BATCH_SIZE * INDICES_PER_QUAD, GL_STATIC_DRAW);
}

void RenderBatch::generateIndexArray() {
    // The pattern of the indices is the same for each quad
    // 0, 1, 2, 0, 3, 2     // then by adding 4 to each index
    // +4,+4,+4,+4,+4,+4
    // 4, 5, 6, 4, 7, 6     // we get this one and so on.
    // The for-loop below generates the indices for each quad

    for (int i = 0; i < BATCH_SIZE; i++) {
        indices[i * 6] = i * 4;
        indices[i * 6 + 1] = i * 4 + 1;
        indices[i * 6 + 2] = i * 4 + 2;
        indices[i * 6 + 3] = i * 4;
        indices[i * 6 + 4] = i * 4 + 3;
        indices[i * 6 + 5] = i * 4 + 2;
    }

}