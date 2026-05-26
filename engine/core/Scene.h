#pragma once

#include "core/GLIncludes.h"
#include "core/GameObject.h"
#include "core/ISystem.h"
#include "core/SceneCamera.h"
#include "core/components/Camera.h"

#include "util/log_error.h"

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
    struct SystemInfo
    {
        std::string name;
        bool enabled = true;
        bool runsInEditMode = false;
        double lastUpdateMilliseconds = 0.0;
    };

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
    std::vector<SystemInfo> getSystemInfo() const;
    bool setSystemEnabled(const std::string &systemName, bool enabled);
    void setAllSystemsEnabled(bool enabled);
    bool isPhysicsEnabled() const;
    void setPhysicsEnabled(bool enabled);
    double getLastPhysicsUpdateMilliseconds() const;
    bool isAnimationEnabled() const;
    void setAnimationEnabled(bool enabled);
    double getLastAnimationUpdateMilliseconds() const;
    void addRigidBody2DToWorld();
    void setGravity(glm::vec2 gravity);

    std::vector<GameObject> &getGameObjects();
    GameObject *findPrimaryCamera();

    const std::string &getTag() const;
    bool serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);

    Physics2D &getPhysics2d();

private:
    struct SystemEntry
    {
        std::string name;
        std::shared_ptr<ISystem> system;
        bool enabled = true;
        double lastUpdateMilliseconds = 0.0;
    };

    // Remove any GameObject wrappers whose underlying entt entity was destroyed.
    void cleanupDestroyedGameObjects();
    void bindRigidBodyDestroyCallback();
    void onRigidBodyDestroyed(entt::registry &registry, entt::entity entity);
    bool m_play = false;

    std::string mTag;
    std::string m_playerTag = "Player";

    entt::registry m_registry;
    Physics2D m_physicsWorld{{0.0f, -9.8f}};
    std::vector<std::shared_ptr<Texture>> m_textures;

    std::vector<GameObject> m_gameObjects;

    std::vector<SystemEntry> m_systems;
    bool m_physicsEnabled = true;
    bool m_animationEnabled = true;
    double m_lastPhysicsUpdateMilliseconds = 0.0;
    double m_lastAnimationUpdateMilliseconds = 0.0;
};
