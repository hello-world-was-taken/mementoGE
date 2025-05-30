#include "core/Camera.h"
#include "core/GameObject.h"

#include "opengl/Vertex.h"

#include "renderer/RenderBatch.h"

#include <memory>

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    void render();

    void updateVertices();
    void generateIndexArray();

    void setCamera(std::shared_ptr<Camera> & camera);
    void setActiveGameObjects(std::vector<GameObject> *gameObjects);

private:
    std::shared_ptr<Camera> m_camera;
    std::unique_ptr<RenderBatch> m_batch;
    std::vector<GameObject> *m_gameObjects;

    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};