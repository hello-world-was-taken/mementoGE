#include "renderer/GridRenderer.h"
#include "core/Resource.h"
#include "opengl/Vertex.h"
#include "util/log_error.h"

GridRenderer::GridRenderer(int width, int height, int tileSize)
    : m_width(width), m_height(height), m_tileSize(tileSize)
{
    generateGridLines();

    m_vao = new VertexArray();
    m_vao->bind();

    // Create VBO with sufficient size and draw type
    m_vbo = new VertexBuffer(m_vertices.size() * sizeof(Vertex), GL_STATIC_DRAW);
    m_vbo->bind();

    m_vao->attachVertexAttribute(VertexAttribute{3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0}); // position

    m_vbo->updateBufferData(m_vertices);

    m_shader = Resource::getShaderProgram("../assets/shader/grid_vertex.shader", "../assets/shader/grid_fragment.shader");
}

GridRenderer::~GridRenderer()
{
    delete m_vbo;
    delete m_vao;
}

void GridRenderer::generateGridLines()
{
    m_vertices.clear();

    // Default color or texcoord, unused in this case
    glm::vec4 color = glm::vec4(1.0f); // optional
    glm::vec2 texCoord = glm::vec2(0.0f);
    float texIndex = 0.0f;

    for (int x = 0; x <= m_width; x += m_tileSize)
    {
        m_vertices.push_back(Vertex{glm::vec3(x, 0, 0), color, texCoord, texIndex});
        m_vertices.push_back(Vertex{glm::vec3(x, m_height, 0), color, texCoord, texIndex});
    }

    for (int y = 0; y <= m_height; y += m_tileSize)
    {
        m_vertices.push_back(Vertex{glm::vec3(0, y, 0), color, texCoord, texIndex});
        m_vertices.push_back(Vertex{glm::vec3(m_width, y, 0), color, texCoord, texIndex});
    }
}

void GridRenderer::render(std::shared_ptr<Camera> camera)
{
    m_shader->use();
    m_shader->setUniform4fv("u_view_matrix", camera->getViewMatrix());
    m_shader->setUniform4fv("u_projection_matrix", camera->getProjectionMatrix());

    m_vao->bind();

    glClearError();
    glDrawArrays(GL_LINES, 0, m_vertices.size());
    glCheckError("glDrawLines", __FILE__, __LINE__);

    m_vao->unbind();
}
