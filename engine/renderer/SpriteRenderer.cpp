#include "core/components/Sprite.h"

#include "renderer/SpriteRenderer.h"

SpriteRenderer::SpriteRenderer()
{
    generateIndexArray();
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::render(const Camera &camera, const std::vector<GameObject> &gameObjects)
{
    if (m_batch == nullptr)
        m_batch = std::make_unique<RenderBatch>(m_indices, GL_TRIANGLES);

    updateVertices(camera, gameObjects);
    m_batch->setVertexData(m_vertices);
    m_batch->setIndexData(m_indices);
    m_batch->render(camera, m_vertices.size() / 4 * 6);
}

// TODO: once we start to use more textures and exceed the amount we can bind to openGL
// at a time, we need to batch our calls per texture units
void SpriteRenderer::updateVertices(const Camera &camera, const std::vector<GameObject> &gameObjects)
{
    m_vertices.clear();

    for (const GameObject &gameObject : gameObjects)
    {
        // The world coordinate is model matrix * local quad.
        std::array<glm::vec3, 4> transformedQuad = gameObject.getWorldCoordinateQuad();
        auto makeVertex = [&](const glm::vec3 &pos, const glm::vec4 &color, const glm::vec2 &texCoord,
                              unsigned int texUnit) -> Vertex
        {
            return Vertex{pos, color, texCoord, float(texUnit)};
        };

        if (gameObject.hasComponent<Sprite>())
        {
            Sprite sprite = gameObject.getComponent<Sprite>();
            auto [topLeft, bottomLeft, bottomRight, topRight] = transformedQuad;

            auto texCoords = sprite.getNormalizedTextureCoordinates();
            float textureUnit = static_cast<float>(sprite.texture->getTextureUnit());

            // Push all 4 vertices — one for each corner
            m_vertices.push_back(makeVertex(topLeft, sprite.color, texCoords[0], textureUnit));
            m_vertices.push_back(makeVertex(bottomLeft, sprite.color, texCoords[1], textureUnit));
            m_vertices.push_back(makeVertex(bottomRight, sprite.color, texCoords[2], textureUnit));
            m_vertices.push_back(makeVertex(topRight, sprite.color, texCoords[3], textureUnit));
        }
    }
}

// The pattern of the indices is the same for each quad
// 0, 1, 2, 0, 3, 2     // then by adding 4 to each index
// +4,+4,+4,+4,+4,+4
// 4, 5, 6, 4, 7, 6     // we get this one and so on.
// The for-loop below generates the indices for each quad
void SpriteRenderer::generateIndexArray()
{
    // m_indices.reserve(m_batch->BATCH_SIZE * m_batch->INDICES_PER_QUAD);
    // for (int i = 0; i < m_batch->BATCH_SIZE; i++)
    // {
    m_indices.resize(1000 * 6);
    for (int i = 0; i < 1000; i++)
    {
        m_indices[i * 6] = i * 4;
        m_indices[i * 6 + 1] = i * 4 + 1;
        m_indices[i * 6 + 2] = i * 4 + 2;
        m_indices[i * 6 + 3] = i * 4;
        m_indices[i * 6 + 4] = i * 4 + 3;
        m_indices[i * 6 + 5] = i * 4 + 2;
    }
    std::cout << "index size: " << m_indices.size() << std::endl;
}
