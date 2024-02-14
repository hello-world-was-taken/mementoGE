#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "draw.h"
#include "shader/create_shader.h"
#include "../util/log_error.h"

// Define vertices for a triangle
std::vector<Vertex> vertices = {
    { -0.5f,  0.5f, 0.0f }, // top left
    { -0.5f, -0.5f, 0.0f }, // bottom left
    {  0.5f, -0.5f, 0.0f }, // bottom right
    {  0.5f,  0.5f, 0.0f }, // top right
};

// Define indices for the vertices
unsigned int indices[] = {
    0, 1, 2,
    0, 3, 2
};

GLuint vao; // New VAO variable
GLuint vbo; // New VBO variable

// Function to generate, bind and allocate to the vertex buffer object and vertex array object
void setBufferData() {
    glGenVertexArrays(1, &vao); // Generate VAO
    glBindVertexArray(vao); // Bind VAO

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Use index buffer to specify the order of vertices
    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Specify vertex attribute pointer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Unbind the VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawTriangle(GLFWwindow* window) {
    setBufferData();
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao); // Bind VAO
    
    glUseProgram(createShaderProgram());
    
    glClearError();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    // glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glCheckError("glDrawArrays", __FILE__, __LINE__);

    glBindVertexArray(0); // Unbind VAO

    glfwSwapBuffers(window);
}