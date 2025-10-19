#pragma once

#include "core/GLIncludes.h"
#include "core/GameObject.h"
#include "core/SceneCamera.h"
#include "core/EnemySystem.h"

#include "util/log_error.h"

#include "opengl/Shader.h"
#include "opengl/Texture.h"

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
    SceneCamera m_sceneCamera;
    EnemySystem m_enemySystem;

    std::vector<std::shared_ptr<Texture>> m_textures;
    // TODO: move this out to editor context
    std::optional<entt::entity> m_activeEntityId;
    std::string m_playerTag = "Player";
    std::string mTag;
};