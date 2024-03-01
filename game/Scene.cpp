#include "Scene.h"

void Scene::start(GLFWwindow *window)
{
    setBufferData();
    draw(window);
}

void Scene::update(float deltaTime, GLFWwindow *window)
{
    draw(window);
    // for (auto &gameObject : gameObjects)
    // {
    //     // update game objects
    // }
}

// Function to generate, bind and allocate to the vertex buffer object and vertex array object
void Scene::setBufferData()
{
    // TODO: Vertex Array should have its own class in the engine
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

    // set the view matrix
    glm::mat4 u_view_matrix = m_camera.getViewMatrix();
    shader.setUniform4fv("u_view_matrix", u_view_matrix);

    // set the projection matrix
    glm::mat4 u_projection_matrix = m_camera.getProjectionMatrix();
    shader.setUniform4fv("u_projection_matrix", u_projection_matrix);

    // move the texture object
    static float u_offset = 0.0f;

    // TODO: we shouldn't be listening for arrow key events here
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

GameObject* Scene::addGameObject()
{
    GameObject *gameObject = new GameObject(m_registry);
    gameObjects.push_back(*gameObject);
    gameObject->addComponent<Transform>();  // every game object has a transform
    return gameObject;
}