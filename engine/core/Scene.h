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

    void update();

    void play();
    void pause();
    void animate();

    GameObject &getPlayer();
    Scene clone(std::string tag);

    GameObject &addGameObject(unsigned int width, unsigned int height, std::string &&tag);
    void removeGameObject(entt::entity);

    void addSystem(const std::string &systemName);
    void addRigidBody2DToWorld();
    void setGraivty(glm::vec2 gravity);

    std::vector<GameObject> &getGameObjects();
    SceneCamera &getCamera();

    const std::string &getTag() const;
    bool serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);

    Physics2D &getPhysics2d();

private:
    bool m_play;

    entt::registry m_registry;
    Physics2D m_physicsWorld{{0.0f, -9.8f}};
    SceneCamera m_sceneCamera;

    std::vector<GameObject> m_gameObjects;
    std::vector<std::unique_ptr<ISystem>> m_systems;
    std::vector<std::string> m_systemNames;
    std::vector<std::shared_ptr<Texture>> m_textures;
    // TODO: move this out to editor context
    std::optional<entt::entity> m_activeEntityId;
    std::string m_playerTag = "Player";
    std::string mTag;
};
