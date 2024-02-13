#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "draw.h"
#include "shader/create_shader.h"
#include "../util/log_error.h"

// Define vertices for a triangle
std::vector<Vertex> vertices = {
    {  0.0f,  1.0f, 0.0f },  // top
    { -1.0f, -1.0f, 0.0f }, // bottom left
    {  1.0f, -1.0f, 0.0f }, // bottom right
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
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glCheckError("glDrawArrays", __FILE__, __LINE__);

    glBindVertexArray(0); // Unbind VAO

    glfwSwapBuffers(window);
}