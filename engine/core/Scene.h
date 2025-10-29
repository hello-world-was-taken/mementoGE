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
#include <imgui.h>
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

    Scene clone(std::string tag);

    void update();

    void play();
    void pause();

    void animate();

    GameObject &getPlayer();

    void addGameObject(unsigned int width, unsigned int height, std::string &&tag);
    void removeGameObject(entt::entity);
    void setActiveGameObject(entt::entity entityId);

    template <typename T, typename... Args>
    void addSystem(Args &&...args); // for 90% cases we won't have system initialization logic, so use this.
    void registerSystem(std::unique_ptr<ISystem> system);

    void addRigidBody2DToWorld();
    void setGraivty(glm::vec2 gravity);

    const std::vector<GameObject> &getGameObjects();
    SceneCamera &getCamera();

    // TODO: move this out to editor context
    GameObject *getActiveGameObject();

    const std::string &getTag() const;
    bool serialize(YAML::Emitter &out);

    Physics2D &getPhysics2d();

  private:
    bool m_play;

    Physics2D m_physicsWorld{{0.0f, -9.8f}};

    entt::registry m_registry;
    std::vector<GameObject> m_gameObjects;
    std::vector<std::unique_ptr<ISystem>> m_systems;
    SceneCamera m_sceneCamera;

    std::vector<std::shared_ptr<Texture>> m_textures;
    // TODO: move this out to editor context
    std::optional<entt::entity> m_activeEntityId;
    std::string m_playerTag = "Player";
    std::string mTag;
};

template <typename T, typename... Args> void Scene::addSystem(Args &&...args)
{
    static_assert(std::is_base_of_v<ISystem, T>, "T must derive from ISystem");
    T system = std::make_unique<T>(std::forward<Args>(args)...);
    m_systems.emplace_back(std::move(system));
}