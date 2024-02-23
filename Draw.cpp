#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Draw.h"
#include "CreateShader.h"
#include "util/log_error.h"
#include "VertexBuffer.h"
#include "Indexbuffer.h"
#include "Texture.h"


// Define vertices for a triangle
std::vector<Vertex> vertices = {
    // positions         // texture
    { -0.5f,  0.5f, 0.0f , 0.0f, 1.0f}, // top left
    { -0.5f, -0.5f, 0.0f , 0.0f, 0.0f}, // bottom left
    {  0.5f, -0.5f, 0.0f , 1.0f, 0.0f}, // bottom right
    {  0.5f,  0.5f, 0.0f , 1.0f, 1.0f}, // top right
};

// Define indices for the vertices
unsigned int indices[] = {
    0, 1, 2,
    0, 3, 2
};

GLuint vao; // New VAO variable

// Function to generate, bind and allocate to the vertex buffer object and vertex array object
void setBufferData() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    VertexBuffer vb(vertices, GL_STATIC_DRAW);

    // Specify vertex attribute pointer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Specify texture attribute pointer
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texture_x));

    IndexBuffer ib(indices, 6, GL_STATIC_DRAW);
}

void drawTriangle(GLFWwindow* window) {
    setBufferData();
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao); // Bind VAO
    
    unsigned int shader_program = createShaderProgram();
    glUseProgram(shader_program);

    // use uniform color
    // static float red = 0.0f;
    // static float increament = 0.5f;

    // if (red > 0)
    //     increament = -0.5f;
    // else
    //     increament = 0.5f;
    // red += increament;

    // int vertex_color_location = glGetUniformLocation(shader_program, "our_color");
    // glUniform4f(vertex_color_location, red, 0.5f, 0.5f, 1.0f);

    // use uniform texture
    const char* texture_path = "../assets/texture/slice01_01.png";
    Texture texture(texture_path);
    texture.bind();
    int texture_location = glGetUniformLocation(shader_program, "our_texture");
    glUniform1i(texture_location, 0);
    
    glClearError();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glCheckError("glDrawArrays", __FILE__, __LINE__);

    glBindVertexArray(0); // Unbind VAO

    glfwSwapBuffers(window);
}