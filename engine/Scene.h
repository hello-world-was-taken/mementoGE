#pragma once

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <entt/entt.hpp>
#include <memory>

#include "util/log_error.h"
#include "engine/Shader.h"
#include "engine/SpriteRenderer.h"
#include "engine/GameObject.h"
#include "engine/Transform.h"
#include "engine/Camera.h"

#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "engine/RenderBatch.h"

class Scene {
private:
    float m_screen_width = (float)(32 * 16); // 16 tiles of 32 pixels
    float m_screen_height = (float)(32 * 9); // 9 tiles of 32 pixels
    entt::registry m_registry;
    // TODO: use type alias to make this more readable
    std::shared_ptr<std::vector<std::shared_ptr<GameObject>>> m_gameObjects;
    Camera m_camera = Camera(m_screen_width, m_screen_height);
    RenderBatch* m_renderBatch = nullptr;
    std::vector<std::shared_ptr<Texture>> m_textures;

public:
    Scene();
    ~Scene();

    void start(GLFWwindow *window);
    void update(float deltaTime, GLFWwindow *window);
    GameObject* addGameObject();
    Camera* getCamera();
};