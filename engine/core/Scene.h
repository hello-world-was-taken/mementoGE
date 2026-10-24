#pragma once

#include "core/GLIncludes.h"
#include "core/GameObject.h"
#include "core/ISystem.h"
#include "core/Physics2D.h"
#include "core/SystemGraph.h"
#include "core/components/Camera.h"

#include "util/log_error.h"

#include "opengl/Texture.h"

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

class SystemScheduler;

class Scene
{
public:
    struct SystemInfo
    {
        std::string name;
        bool enabled = true;
        double lastUpdateMilliseconds = 0.0;
    };

    Scene(std::string &&tag);
    Scene(const YAML::Node &&serializedScene);
    ~Scene();

    // We can't use a copy constructor because entt::registry is not copyable
    Scene(const Scene &other) = delete; // copy constructor
    Scene(Scene &&other) noexcept;      // move constructor
    Scene &operator=(Scene &&other);    // move assignment operator

    // Both overloads finish all accepted updates before returning or throwing.
    // The owner must keep this scene and frame alive and avoid external access
    // during the tick. The overload without a scheduler executes inline.
    void update(const FrameContext &frame, bool editMode = false);
    void update(const FrameContext &frame, SystemScheduler &scheduler, bool editMode = false);

    void play();
    void pause();
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
    void addRigidBody2DToWorld();
    void setGravity(glm::vec2 gravity);

    std::vector<GameObject> &getGameObjects();
    const std::vector<GameObject> &getGameObjects() const;
    GameObject *findGameObjectByUuid(uint64_t uuid);
    const GameObject *findGameObjectByUuid(uint64_t uuid) const;
    GameObject *findPrimaryCamera();
    const GameObject *findPrimaryCamera() const;

    const std::string &getTag() const;
    bool serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);

    Physics2D &getPhysics2d();

private:
    struct SystemEntry
    {
        std::string name;           // Factory name used by configuration and serialization.
        std::string descriptorName; // Identity used by the dependency graph.
        std::unique_ptr<ISystem> system;
        bool enabled = false;
        double lastUpdateMilliseconds = 0.0;
    };

    void addSystemEntry(std::string name, std::unique_ptr<ISystem> system);
    // Remove any GameObject wrappers whose underlying entt entity was destroyed.
    void cleanupDestroyedGameObjects();
    void bindRigidBodyDestroyCallback();
    void onRigidBodyDestroyed(entt::registry &registry, entt::entity entity);
    bool m_play = false;

    std::string mTag;
    std::string m_playerTag = "Player";

    entt::registry m_registry;
    glm::vec2 m_gravity{0.0f, -9.8f};
    Physics2D m_physicsWorld{m_gravity};
    std::vector<std::shared_ptr<Texture>> m_textures;

    std::vector<GameObject> m_gameObjects;

    std::vector<SystemEntry> m_systems;
    SystemGraph m_systemGraph;
};
