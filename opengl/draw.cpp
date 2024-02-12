#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct Vertex {
    float x, y, z;
};

// Define vertices for a triangle
std::vector<Vertex> vertices = {
    {  0.0f,  1.0f, 0.0f },  // top
    { -1.0f, -1.0f, 0.0f }, // bottom left
    {  1.0f, -1.0f, 0.0f }, // bottom right
};

GLuint vbo;

// Function to generate, bind and allocate to the vertex buffer object
void setBufferData() {
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // Unbind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawTriangle(GLFWwindow* window) {
    setBufferData();
    glClear(GL_COLOR_BUFFER_BIT);

    // rebounding the buffer since openGL is a state machine and we might have unbound it or bound another buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    // Enable vertex attribute array
    glEnableVertexAttribArray(0);
    // Specify vertex attribute pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glEnableVertexAttribArray(0);

    // Unbind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glfwSwapBuffers(window);
}