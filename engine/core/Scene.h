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
#include "engine/core/GameObject.h"
#include "engine/core/Transform.h"
#include "engine/core/Camera.h"
#include "engine/renderer/RenderBatch.h"

class Scene
{
public:
    Scene();
    ~Scene();
    // We can't use a copy constructor because entt::registry is not copyable
    Scene(const Scene &other) = delete;        // copy constructor
    Scene(Scene &&other) = default;            // move constructor
    Scene &operator=(Scene &&other) = default; // move assignment operator

    void start();
    void update(float deltaTime, GLFWwindow *window);
    std::shared_ptr<GameObject> addGameObject(unsigned int width, unsigned int height);
    std::shared_ptr<std::vector<std::shared_ptr<GameObject>>> getGameObjects();
    // TODO: think more about this
    // void addTextureToGameObject(std::shared_ptr<GameObject> gameObject, std::shared_ptr<Texture> texture);
    std::shared_ptr<Camera> getCamera();
    void renderActiveGameObjectPropsImGui();

private:
    float m_screen_width = 32.0f * 16.0f; // 16 tiles of 32 pixels = 512 pixels
    float m_screen_height = 32.0f * 9.0f; // 9 tiles of 32 pixels = 288 pixels
    entt::registry m_registry;
    // TODO: use type alias to make this more readable
    std::shared_ptr<std::vector<std::shared_ptr<GameObject>>> m_gameObjects;
    std::shared_ptr<Camera> m_camera = std::make_shared<Camera>(m_screen_width, m_screen_height);
    RenderBatch *m_renderBatch = nullptr;
    std::vector<std::shared_ptr<Texture>> m_textures;
    GameObject *m_activeGameObject = nullptr;
};