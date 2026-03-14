#pragma once

#include "core/GLIncludes.h"
#include "core/GameObject.h"
#include "core/ISystem.h"
#include "core/SceneCamera.h"

#include "util/log_error.h"

#include "opengl/Shader.h"
#include "opengl/Texture.h"

#include "physics/Physics2D.h"

#include <entt/entt.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
#include <yaml-cpp/yaml.h>

class Scene
{
public:
    Scene(std::string &&tag);
    Scene(const YAML::Node &&serializedScene);
    ~Scene();

    // We can't use a copy constructor because entt::registry is not copyable
    Scene(const Scene &other) = delete; // copy constructor
    Scene(Scene &&other) noexcept;      // move constructor
    Scene &operator=(Scene &&other);    // move assignment operator

    void update();

    void play();
    void pause();
    void animate();

    GameObject &getPlayer();
    Scene clone(std::string tag);

    GameObject &addGameObject(unsigned int width, unsigned int height, std::string &&tag);
    GameObject &addGameObjectFromModel(const std::string &modelPath);
    GameObject &addGameObjectFromSerialized(const YAML::Node &serializedGameObject);
    void removeGameObject(entt::entity);

    void addSystem(const std::string &systemName);
    void addRigidBody2DToWorld();
    void setGravity(glm::vec2 gravity);

    std::vector<GameObject> &getGameObjects();
    SceneCamera &getCamera();

    const std::string &getTag() const;
    bool serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);

    Physics2D &getPhysics2d();

private:
    // Remove any GameObject wrappers whose underlying entt entity was destroyed.
    void cleanupDestroyedGameObjects();

    bool m_play = false;

    std::string mTag;
    std::string m_playerTag = "Player";

    entt::registry m_registry;
    Physics2D m_physicsWorld{{0.0f, -9.8f}};
    std::vector<std::shared_ptr<Texture>> m_textures;

    SceneCamera m_sceneCamera;
    std::vector<GameObject> m_gameObjects;

    std::vector<std::shared_ptr<ISystem>> m_systems;
    std::vector<std::string> m_systemNames;
};
