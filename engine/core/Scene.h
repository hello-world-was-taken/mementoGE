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
#include <string>
#include <yaml-cpp/yaml.h>

#include "util/log_error.h"
#include "opengl/Shader.h"
#include "core/GameObject.h"
#include "core/Transform.h"
#include "core/Camera.h"
#include "renderer/RenderBatch.h"
#include "renderer/GridRenderer.h"

class Scene
{
public:
    Scene(std::string &&tag);
    Scene(const YAML::Node &&serializedScene);
    ~Scene();

    // We can't use a copy constructor because entt::registry is not copyable
    Scene(const Scene &other) = delete; // copy constructor
    Scene(Scene &&other);               // move constructor
    Scene &operator=(Scene &&other);    // move assignment operator

    void start();
    void update(float deltaTime, GLFWwindow *window);
    GameObject &addGameObject(unsigned int width, unsigned int height, std::string &&tag);
    std::vector<GameObject> &getGameObjects();
    // TODO: think more about this
    // void addTextureToGameObject(std::shared_ptr<GameObject> gameObject, std::shared_ptr<Texture> texture);
    std::shared_ptr<Camera> getCamera();
    void renderActiveGameObjectPropsImGui();
    GameObject &getActiveGameObject();
    bool setActiveGameObject(GameObject *gameObject);
    const std::string &getTag() const;
    bool serialize(YAML::Emitter &out);

private:
    float m_screen_width = 32.0f * 16.0f; // 16 tiles of 32 pixels = 512 pixels
    float m_screen_height = 32.0f * 9.0f; // 9 tiles of 32 pixels = 288 pixels
    entt::registry m_registry;
    // TODO: use type alias to make this more readable
    std::vector<GameObject> m_gameObjects;
    // TODO: the camera should use the framebuffer not the fixed screen width and height
    std::shared_ptr<Camera> m_camera = std::make_shared<Camera>(m_screen_width, m_screen_height);
    RenderBatch *m_renderBatch = nullptr;
    // TODO: the grid renderer should use the framebuffer not the fixed screen width and height
    GridRenderer mGridRenderer{static_cast<int>(m_screen_width), static_cast<int>(m_screen_height), 32}; // TODO: improve it
    std::vector<std::shared_ptr<Texture>> m_textures;
    GameObject *m_activeGameObject = nullptr;
    std::string mTag;
};