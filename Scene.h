#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "util/log_error.h"
#include "VertexBuffer.h"
#include "Indexbuffer.h"
#include "Texture.h"
#include "Shader.h"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "Camera.h"

class Scene {
private:
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
    void setBufferData();
    void draw(GLFWwindow *window);
};