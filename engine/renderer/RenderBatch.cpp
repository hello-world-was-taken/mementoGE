#include <iostream>
#include <GL/glew.h>
#include <memory>

#include "engine/renderer/RenderBatch.h"
#include "engine/core/Resource.h"
#include "engine/core/Sprite.h"
#include "util/log_error.h"

RenderBatch::RenderBatch(
    const std::shared_ptr<Camera> camera,
    std::shared_ptr<std::vector<std::shared_ptr<GameObject>>> gameObjects)
    : m_camera(camera), m_gameObjects(gameObjects)
{
    generateIndexArray();
    generateVertexBuffer();
}

RenderBatch::~RenderBatch()
{
    std::cout << "RenderBatch destructor called" << std::endl;
    glDeleteVertexArrays(1, &this->m_vao);

    mp_vb->unbind();
    mp_ib->unbind();

    delete mp_vb;
    delete mp_ib;
}

void RenderBatch::render()
{
    glBindVertexArray(m_vao); // Bind VAO

    mp_vb->bind();

    // repopulate the vertices with the new data
    updateVertexBuffer();

    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: We shouldn't me creating a new shader every frame
    std::shared_ptr<Shader> shader = Resource::getShaderProgram("../assets/shader/vertex.shader", "../assets/shader/fragment.shader");
    shader.get()->use();

    // set the view matrix
    glm::mat4 u_view_matrix = m_camera->getViewMatrix();
    shader.get()->setUniform4fv("u_view_matrix", u_view_matrix);

    // set the projection matrix
    glm::mat4 u_projection_matrix = m_camera->getProjectionMatrix();
    shader.get()->setUniform4fv("u_projection_matrix", u_projection_matrix);

    shader.get()->setMultipleTextureUnits("textures", m_texture_units.data(), m_texture_units.size());

    glClearError();
    glDrawElements(GL_TRIANGLES, BATCH_SIZE * INDICES_PER_QUAD, GL_UNSIGNED_INT, nullptr);
    glCheckError("glDrawArrays", __FILE__, __LINE__);

    glBindVertexArray(0); // Unbind VAO
}

void RenderBatch::updateVertexBuffer()
{
    // clear the vertices vector
    m_vertices.clear();

    for (std::shared_ptr<GameObject> gameObject : *m_gameObjects)
    {

        Transform transform = gameObject->getComponent<Transform>();

        glm::mat4x4 transformMatrix = transform.getTransformMatrix();
        std::vector<glm::vec3> transformedQuad = transformQuad(transformMatrix, gameObject->getQuad());

        if (gameObject->hasComponent<Sprite>())
        {
            Sprite sprite = gameObject->getComponent<Sprite>();

            for (int i = 0; i < transformedQuad.size(); i++)
            {
                m_vertices.push_back({transformedQuad[i],
                                    sprite.getColor(),
                                    sprite.getTextureCoordinates()[i], // TODO: do we need to retrieve this from the sprite renderer?
                                    (float)sprite.getTexture()->getTextureUnit()});
            }
        }
    }

    // update the vertex buffer
    mp_vb->updateBufferData(m_vertices);
}

std::vector<glm::vec3> RenderBatch::transformQuad(glm::mat4x4 transformMatrix, std::vector<glm::vec3> quad)
{
    std::vector<glm::vec3> transformedQuad = quad;
    for (int i = 0; i < quad.size(); i++)
    {
        transformedQuad[i] = transformMatrix * glm::vec4(quad[i], 1.0f);
    }

    return transformedQuad;
}

// TODO: why is this function creating an index buffer?
void RenderBatch::generateVertexBuffer()
{
    // 1000 Quads * 4 vertices per quad * sizeof(Vertex)
    unsigned int bufferSize = 1000 * 4 * sizeof(Vertex);
    glClearError();
    if (glGenVertexArrays == NULL)
    {
        std::cout << "Something is wrong" << &m_vao << std::endl;
    }
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    mp_vb = new VertexBuffer(bufferSize, GL_DYNAMIC_DRAW);
    mp_vb->bind();

    mp_vb->addAttribute(VertexAttribute(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0));
    mp_vb->addAttribute(VertexAttribute(4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color)));
    mp_vb->addAttribute(VertexAttribute(2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texture)));
    mp_vb->addAttribute(VertexAttribute(1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texIndex)));

    // Setting the vertex attribute pointers adds the VBO to the currently bound VAO
    mp_vb->setAttributePointers();

    mp_ib = new IndexBuffer{m_indices, BATCH_SIZE * INDICES_PER_QUAD, GL_STATIC_DRAW};
    mp_ib->bind();
}

void RenderBatch::generateIndexArray()
{
    // The pattern of the indices is the same for each quad
    // 0, 1, 2, 0, 3, 2     // then by adding 4 to each index
    // +4,+4,+4,+4,+4,+4
    // 4, 5, 6, 4, 7, 6     // we get this one and so on.
    // The for-loop below generates the indices for each quad

    for (int i = 0; i < BATCH_SIZE; i++)
    {
        m_indices[i * 6] = i * 4;
        m_indices[i * 6 + 1] = i * 4 + 1;
        m_indices[i * 6 + 2] = i * 4 + 2;
        m_indices[i * 6 + 3] = i * 4;
        m_indices[i * 6 + 4] = i * 4 + 3;
        m_indices[i * 6 + 5] = i * 4 + 2;
    }
}