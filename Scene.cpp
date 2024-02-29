#include "Scene.h"


// Function to generate, bind and allocate to the vertex buffer object and vertex array object
void Scene::setBufferData()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    VertexBuffer vb(vertices, GL_STATIC_DRAW);

    vb.addAttribute(VertexAttribute(3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0));
    vb.addAttribute(VertexAttribute(2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texture_x)));

    // Setting the vertex attribute pointers adds the VBO to the currently bound VAO
    vb.setAttributePointers();

    IndexBuffer ib(indices, 6, GL_STATIC_DRAW);
}

void Scene::draw(GLFWwindow *window)
{
    setBufferData();
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao); // Bind VAO

    Shader shader("../assets/shader/vertex.shader", "../assets/shader/fragment.shader");
    shader.use();

    // Camera
    float screen_width = (float)(32 * 16); // 16 tiles of 32 pixels
    int screen_height = (float)(32 * 9);   // 9 tiles of 32 pixels
    Camera camera(screen_width, screen_height);

    // set the view matrix
    glm::mat4 u_view_matrix = camera.getViewMatrix();
    shader.setUniform4fv("u_view_matrix", u_view_matrix);

    // set the projection matrix
    glm::mat4 u_projection_matrix = camera.getProjectionMatrix();
    shader.setUniform4fv("u_projection_matrix", u_projection_matrix);

    // move the texture object
    static float u_offset = 0.0f;

    // listen for arrow key events
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        u_offset += 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        u_offset -= 1.0f;
    }

    glm::mat4 u_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(u_offset, 0.0f, 0.0f));
    shader.setUniform4fv("u_model_matrix", u_model_matrix);

    // glm::mat4 u_model_matrix = glm::rotate(u_model_matrix, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // shader.setUniform4fv("u_model_matrix", u_model_matrix);

    // use uniform texture
    const char *texture_path = "../assets/texture/slice01_01.png";
    Texture texture(texture_path);
    texture.bind();

    shader.setUniform1i("our_texture", 0);

    glClearError();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glCheckError("glDrawArrays", __FILE__, __LINE__);

    glBindVertexArray(0); // Unbind VAO
}