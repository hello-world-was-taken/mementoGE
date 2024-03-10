#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <imgui.h>

#include "util/log_error.h"
#include "engine/opengl/Shader.h"
#include "engine/renderer/SpriteRenderer.h"
#include "engine/core/GameObject.h"
#include "engine/core/Transform.h"
#include "engine/core/Camera.h"
#include "engine/renderer/RenderBatch.h"

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
    GameObject* m_activeGameObject = nullptr;

public:
    Scene();
    ~Scene();

    void start(GLFWwindow *window);
    void update(float deltaTime, GLFWwindow *window);
    std::shared_ptr<GameObject> addGameObject(unsigned int width, unsigned int height);
    // TODO: think more about this
    // void addTextureToGameObject(std::shared_ptr<GameObject> gameObject, std::shared_ptr<Texture> texture);
    Camera* getCamera();
    void renderActiveGameObjectPropsImGui();
};