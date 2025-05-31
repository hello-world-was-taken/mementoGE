#pragma once

#include "util/log_error.h"
#include "opengl/Shader.h"

#include "renderer/SpriteRenderer.h"

#include "core/GLIncludes.h"
#include "core/GameObject.h"
#include "core/Transform.h"
#include "core/Camera.h"

#include "physics/Physics2D.h"

#include <iostream>
#include <vector>
#include <memory>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <imgui.h>
#include <string>
#include <yaml-cpp/yaml.h>
#include <optional>

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

    Scene clone(std::string tag);

    void start();
    void update(float deltaTime, GLFWwindow *window);

    void setPaused(bool paused);
    bool isPaused() const;

    void addGameObject(unsigned int width, unsigned int height, std::string &&tag);
    void setActiveGameObject(entt::entity entityId);

    void addRigidBody2DToWorld();

    std::vector<GameObject> &getGameObjects();
    std::shared_ptr<Camera> getCamera() const;
    GameObject *getActiveGameObject();

    const std::string &getTag() const;
    bool serialize(YAML::Emitter &out);

    // TODO: should be private. For testing purposes
    Physics2D m_physicsWorld{{0.0f, -9.8f}};

private:
    // logical game world screen size
    float m_screen_width = 32.0f * 16.0f; // 16 tiles of 32 pixels = 512 pixels
    float m_screen_height = 32.0f * 9.0f; // 9 tiles of 32 pixels = 288 pixels

    bool m_isPaused;

    // Physics2D m_physicsWorld{{0.0f, 9.8f}};
    SpriteRenderer m_spriteRenderer{};

    entt::registry m_registry;
    std::vector<GameObject> m_gameObjects;
    std::shared_ptr<Camera> m_camera = std::make_shared<Camera>(m_screen_width, m_screen_height);

    std::vector<std::shared_ptr<Texture>> m_textures;
    std::optional<entt::entity> m_activeEntityId;
    std::string mTag;
};