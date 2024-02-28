#include "Draw.h"


// Define vertices for a triangle
std::vector<Vertex> vertices = {
    // positions                // texture
    { -0.5f,  0.5f, 0.0f ,      0.0f, 1.0f}, // top left
    { -0.5f, -0.5f, 0.0f ,      0.0f, 0.0f}, // bottom left
    {  0.5f, -0.5f, 0.0f ,      1.0f, 0.0f}, // bottom right
    {  0.5f,  0.5f, 0.0f ,      1.0f, 1.0f}, // top right
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
    
    vb.addAttribute(VertexAttribute(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0));
    vb.addAttribute(VertexAttribute(2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texture_x)));
    
    // Setting the vertex attribute pointers adds the VBO to the currently bound VAO
    vb.setAttributePointers();

    IndexBuffer ib(indices, 6, GL_STATIC_DRAW);
}

void drawTriangle(GLFWwindow* window) {
    setBufferData();
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao); // Bind VAO
    
    Shader shader("../assets/shader/vertex.shader", "../assets/shader/fragment.shader");
    shader.use();

    // move the texture object
    static float u_offset = 0.0f;

    // listen for arrow key events
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        u_offset += 0.01f;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        u_offset -= 0.01f;
    }

    glm::mat4 u_model_translation_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(u_offset, 0.0f, 0.0f));
    shader.setUniform4fv("u_model_translation_matrix", u_model_translation_matrix);

    // use uniform texture
    const char* texture_path = "../assets/texture/slice01_01.png";
    Texture texture(texture_path);
    texture.bind();
    
    shader.setUniform1i("our_texture", 0);
    
    glClearError();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glCheckError("glDrawArrays", __FILE__, __LINE__);

    glBindVertexArray(0); // Unbind VAO
}