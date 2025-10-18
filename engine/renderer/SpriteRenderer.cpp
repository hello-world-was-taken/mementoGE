#include "renderer/SpriteRenderer.h"

#include "core/Sprite.h"

SpriteRenderer::SpriteRenderer()
{
    generateIndexArray();
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::render(
    const Camera &camera,
    const std::vector<GameObject> &gameObjects)
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
void SpriteRenderer::updateVertices(
    const Camera &camera,
    const std::vector<GameObject> &gameObjects)
{
    m_vertices.clear();

    for (const GameObject &gameObject : gameObjects)
    {

        Transform transform = gameObject.getComponent<Transform>();

        // The world coordinate is model matrix * local quad.
        std::vector<glm::vec3> transformedQuad = gameObject.getWorldCoordinateQuad();

        if (gameObject.hasComponent<Sprite>())
        {
            Sprite sprite = gameObject.getComponent<Sprite>();
            for (int i = 0; i < transformedQuad.size(); i++)
            {
                // TODO: in scenarios where sprite width and height are different
                // from gameObject, sprite should take precedence. In fact, if a
                // game object has a sprite attached to it, the width and height
                // should be taken from the sprite.
                m_vertices.push_back({transformedQuad[i],
                                      sprite.color,
                                      sprite.getNormalizedTextureCoordinates()[i], // TODO: do we need to retrieve this from the sprite renderer?
                                      (float)sprite.texture->getTextureUnit()});
            }
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
