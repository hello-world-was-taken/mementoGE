#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <entt/entt.hpp>

#include "util/log_error.h"
#include "engine/VertexBuffer.h"
#include "engine/Indexbuffer.h"
#include "engine/Texture.h"
#include "engine/Shader.h"
#include "engine/Camera.h"
#include "engine/SpriteRenderer.h"
#include "engine/GameObject.h"
#include "engine/Transform.h"

#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"

class Scene {
private:
    float m_screen_width = (float)(32 * 16); // 16 tiles of 32 pixels
    float m_screen_height = (float)(32 * 9); // 9 tiles of 32 pixels
    entt::registry m_registry;
    std::vector<GameObject> gameObjects;
    Camera m_camera = Camera(m_screen_width, m_screen_height);

    std::vector<Vertex> vertices = {
        // positions                // texture
        {  0.0f, 100.0f, 0.0f,      0.0f, 1.0f},   // top left
        {  0.0f,   0.0f, 0.0f,      0.0f, 0.0f},     // bottom left
        {100.0f,   0.0f, 0.0f,      1.0f, 0.0f},   // bottom right
        {100.0f, 100.0f, 0.0f,      1.0f, 1.0f}, // top right
    };

    // Define indices for the vertices
    unsigned int indices[6] = {
        0, 1, 2,
        0, 3, 2};

    unsigned int vao; // New VAO variable
public:
    void start(GLFWwindow *window);
    void update(float deltaTime, GLFWwindow *window);
    GameObject* addGameObject();
    void setBufferData();
    void draw(GLFWwindow *window);
};